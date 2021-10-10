/*******************************************************************
	> File Name: server.h
	> Author: 胡孟
	> Mail: 13535324513@163.com
	> Created Time: Sat 25 Sep 2021 08:59:01 PM CST
 ******************************************************************/

#ifndef _SERVER_H
#define _SERVER_H

#include <iostream>
#include <functional>
#include <unordered_map>
#include <memory>
#include <tuple>
#include <cstdlib>
#include <array>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <msgpack.hpp>

#include "rpc_config.h"
#include "rpc_codec.h"
#include "trait.h"

template<typename F, size_t ...i, typename T>
decltype(auto) call_func_helper(F&& func, std::index_sequence<i...>, T&& t){
    return func(std::get<i>(std::forward<T>(t))...);
}

template<typename F, typename T>
decltype(auto) call_func(F&& func, T&& t){
    constexpr auto size = std::tuple_size<typename std::decay<T>::type>::value;
    return call_func_helper(std::forward<F>(func), std::make_index_sequence<size>{}, std::forward<T>(t));
}

/*饿汉模式*/
class rpcServer{
public:
    void init(const char* path, const int threadNums=std::thread::hardware_concurrency()){
        config_ptr = std::make_shared<rpcConfig>();
        config_ptr->load_config_file(path);     
        
        //服务器相关
        service_ptr = std::make_shared<boost::asio::io_service>();
        std::string serverPort = config_ptr->load("rpcserverport");
        if(serverPort.size() == 0)
            throw std::runtime_error("Server port configuration error");
        
        ep_ptr = std::make_shared<boost::asio::ip::tcp::endpoint>(boost::asio::ip::tcp::v4(),atoi(serverPort.c_str()));
        acc_ptr = std::make_shared<boost::asio::ip::tcp::acceptor>(*service_ptr,*ep_ptr);
        code_ptr = std::make_shared<codec>();

        //设置地址复用
        acc_ptr->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        //缓冲区
        read_buf_ptr = std::make_shared<std::array<char, 2048>>();
        write_buf_ptr = std::make_shared<std::array<char, 2048>>();
    }
    
    //客户端处理函数调用
    void call(std::string funcName, const char* data, int len){
        auto func = m_handlers[funcName];
        func(data,len);  //跳转到callproxy
    }

    template<typename F>
    void callproxy(F func, const char* data, int len){
        //把函数参数类型萃取出来
        using args_type = typename function_traits<F>::tuple_type;
        int err=0;
        args_type args = code_ptr->unpack<args_type>(data,len,err);
        //返回包如下，前四个字节表示正确或错误
        //如果为0则表示正确，可再读四个字节代表返回
        //包的大小，若非0则表示错误，该字段表示错误的
        //提示信息字段大小，据此再读出错误信息，抛出异常
        if(err){
            //参数不匹配, 构造错误包裹
            std::string err_msg = "Parameter mismatch";
            send_err(err_msg);    
        }else{
            auto res = ::call_func(func,args);
            send_res(res);
        }
    }

    //类成员函数
    template<typename R, typename C, typename obj, typename... Args>
    void callproxy(R(C::* func)(Args...), obj* s, const char* data, int len){
        using args_type = std::tuple<typename std::decay<Args>::type...>;
        int err = 0;
        args_type args = code_ptr->unpack<args_type>(data,len,err);
        if(err){
            std::string err_msg = "Parameter mismatch";
            try{
                send_err(err_msg);
            }catch(const char* err){
                throw std::runtime_error(err);
            }
        }else{
            //构造匿名函数
            auto ff = [&](Args... ps)->R{
                return (s->*func)(ps...);
            };
            auto res = ::call_func(ff,args);
            try{
                send_res(res);
            }catch(const char* err){
                throw std::runtime_error(err);
            }
        }
    }

    template<typename F, typename obj>
    void callproxy_(F func, obj* self, const char* data, int len){
        callproxy(func,self,data,len);
    }

    //删除映射表中的函数
    void delete_method(const std::string& funcName){
        if(m_handlers.find(funcName) != m_handlers.end())
            m_handlers.erase(funcName);
    }

    //注册函数
    template<typename F>
    void register_method(const std::string funcName, F handler){
        m_handlers[funcName] = std::bind(&rpcServer::callproxy<F>, this, handler, std::placeholders::_1, std::placeholders::_2);
    }

    //成员函数
    template<typename F, typename obj>
    void register_method(const std::string funcName, F handler, obj* self){
        m_handlers[funcName] = std::bind(&rpcServer::callproxy_<F,obj>, this, handler, self, std::placeholders::_1, std::placeholders::_2);
    }
    
    void run(){
        //先实现同步单线程
        while(true){
            socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(*service_ptr);
            acc_ptr->accept(*socket_ptr);
            
            while(true){
                size_t len;
                try{
                    len = socket_ptr->read_some(boost::asio::buffer(read_buf_ptr->data(),2048));
                }catch(...){
                    break;
                }
                
                size_t begin = 0;
                while(begin < len){
                    if(len-begin > 4){
                        size_t funcNameLen = parse_len(read_buf_ptr->data());
                
                        //解出函数名
                        std::string funcName = parse_func_name(read_buf_ptr->data()+4+begin,funcNameLen);
                        if(m_handlers.find(funcName)==m_handlers.end()){
                            //构造错误包
                            std::string err_msg = std::string("The function ") + funcName 
                                + std::string(" has not been registered");
                            try{
                                send_err(err_msg);
                            }catch(const char* err){
                                std::cout << err << std::endl;
                                socket_ptr->close();
                            }
                            break;
                        }
                        std::cout << "from: " << socket_ptr->remote_endpoint().address().to_string() << ":" 
                            << socket_ptr->remote_endpoint().port() << ", call for function \'" 
                            << funcName << "\'" << std::endl; 
                        size_t argsLen = parse_len(read_buf_ptr->data()+4+funcNameLen+begin);
                        try{
                            call(funcName,read_buf_ptr->data()+8+funcNameLen+begin,argsLen);
                        }catch(const char* err){
                            std::cout << err << std::endl;
                            socket_ptr->close();
                        }
                        begin += 8+funcNameLen+argsLen;
                    }
                }
            }
        }
    }
    
    //显示配置
    void show_config(){
        config_ptr->show();
    }

    static rpcServer* make_obj(){
        return &obj;
    }
    
private:

    void send_err(std::string msg){
        auto err = code_ptr->pack(msg);
        size_t len = err.size();
        memcpy(write_buf_ptr->data(), &len, 4);
        memcpy(write_buf_ptr->data()+4, err.data(), err.size());
        boost::system::error_code ec;
        try{
            socket_ptr->write_some(boost::asio::buffer(write_buf_ptr->data(),4+err.size()),ec);
        }catch(...){
            if(ec)
                std::cout << boost::system::system_error(ec).what() << std::endl;
            
            throw std::runtime_error("the socket may have closed");
        }
    }

    template<typename T>
    void send_res(T& res){
        //首字段为0表示成功
        size_t msg_code = 0;
        memcpy(write_buf_ptr->data(), &msg_code, 4);
        auto msg = code_ptr->pack(res);
        size_t len = msg.size();
        memcpy(write_buf_ptr->data()+4, &len, 4);
        memcpy(write_buf_ptr->data()+8, msg.data(), msg.size());
        boost::system::error_code ec;
        try{
        socket_ptr->write_some(boost::asio::buffer(write_buf_ptr->data(), 8+msg.size()), ec);
        }catch(...){
            if(ec)
                std::cout << boost::system::system_error(ec).what() << std::endl;
            
            throw std::runtime_error("the socket may have closed");
        }
    }

    std::string parse_func_name(const char* data, int len){
        return code_ptr->unpack<std::string>(data,len);
    }
    int parse_len(const char* data){
        return code_ptr->unpack<unsigned int>(data,4);
    }

private:
    rpcServer() = default;
    ~rpcServer() = default;
    rpcServer(const rpcServer&) = delete;
    rpcServer(rpcServer&&) = delete;
    rpcServer operator=(const rpcServer&) = delete;
    
    //单例对象
    static rpcServer obj;

    //解析配置文件类
    std::shared_ptr<rpcConfig> config_ptr;

    //服务器相关
    std::shared_ptr<boost::asio::io_service> service_ptr;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acc_ptr;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
    std::shared_ptr<boost::asio::ip::tcp::endpoint> ep_ptr;

    //函数映射表
    std::unordered_map<std::string, std::function<void(const char* data, int len)>> m_handlers;

    //缓冲区
    std::shared_ptr<std::array<char, 2048>> read_buf_ptr;
    std::shared_ptr<std::array<char, 2048>> write_buf_ptr;

    //其他
    std::shared_ptr<codec> code_ptr;
};

rpcServer rpcServer::obj;

#endif
