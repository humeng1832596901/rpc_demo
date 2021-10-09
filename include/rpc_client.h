/*******************************************************************
	> File Name: rpc_client.h
	> Author: 胡孟
	> Mail: 13535324513@163.com
	> Created Time: Tue 28 Sep 2021 08:34:07 AM CST
 ******************************************************************/

#ifndef _RPC_CLIENT_H
#define _RPC_CLIENT_H

#include <memory>
#include <string>
#include <iostream>
#include <cstdlib>
#include <array>

#include <boost/asio.hpp>

#include "rpc_codec.h"

class rpcClient : public std::enable_shared_from_this<rpcClient>{
public:
    rpcClient() = default;
    ~rpcClient(){
        close();
    }
    rpcClient(const std::string& ip_, const unsigned short port_){
        this->init(ip_,port_);
    }
    //服务端ip和port初始化
    void init(const std::string& ip_, const unsigned short port_){
        ip = ip_;
        port = port_;
        service_ptr = std::make_shared<boost::asio::io_service>();
        socket_ptr = std::make_shared<boost::asio::ip::tcp::socket>(*service_ptr);
        ep_ptr = std::make_shared<boost::asio::ip::tcp::endpoint>(boost::asio::ip::address::from_string(ip_),port_);
        work = std::make_shared<boost::asio::io_service::work>(*service_ptr);
        code_ptr = std::make_shared<codec>();        
        write_buf_ptr = std::make_shared<std::array<char,2048>>();
        read_buf_ptr = std::make_shared<std::array<char, 2048>>();
    }

    //连接服务器
    void connect(std::size_t timeout = 2){
        //以下为同步连接
        boost::system::error_code ec;
        socket_ptr->connect(*ep_ptr, ec);
        //设置 socket 为无时延 socket
        static boost::asio::ip::tcp::no_delay option(true);
        socket_ptr->set_option(option);
        if(ec)
            std::cerr << boost::system::system_error(ec).what() << std::endl;
        
    }
    
    template<typename T, typename... Args>
        typename std::enable_if<!std::is_void<T>::value, T>::type call(const std::string& funcName, Args&&... args){
            //构造数据包
            size_t len = construct_packet(funcName, std::forward<Args>(args)...);
            
            boost::system::error_code ignored_ec;
            socket_ptr->write_some(boost::asio::buffer(write_buf_ptr->data(),len),ignored_ec);
            len = socket_ptr->read_some(boost::asio::buffer(read_buf_ptr->data(),2048));
            
            //codec code;
            //包的格式有两种，先解析前四个字节，若值为0则代表调用成功，再解析四个字节代表结果的长度，读取后
            //再返回给调用者即可
            //如果前四个字节的值非零，则出错，其大小表示错误信息的长度，再读取后解析跑出异常，当然，
            //更省事的方法是在client维护一个error的哈希表，不同的错误码代表不同的错误类型
            //这样可以省去网络传输错误信息的开销,当然这样灵活多比较低，新增一种错误类型，客户端代码
            //也要重新编译
            int t;
            T res;
            size_t flag = code_ptr->unpack<size_t>(read_buf_ptr->data(),4,t);
            if(t){
                //网络传输数据出错，可能是丢包之类的, 抛异常
                throw std::runtime_error("Network data transmission error");
            }

            if(flag != 0){
                //处理错误信息
                std::string err_msg = code_ptr->unpack<std::string>(read_buf_ptr->data()+4, flag);
                //抛异常
                throw std::runtime_error(err_msg);
            }else{
                size_t len = code_ptr->unpack<size_t>(read_buf_ptr->data()+4, 4);
                res = code_ptr->unpack<T>(read_buf_ptr->data()+8, len);
            }
            return res;
    }

    void close(){
        boost::system::error_code ec;
        if(!has_connected)
            return;
        has_connected = false;
        socket_ptr->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_ptr->close(ec);
    }

private:

    template<typename... Args>
    size_t construct_packet(const std::string funcName, Args&&... args){
            //序列化函数名
            codec obj;
            msgpack::sbuffer name = obj.pack(funcName);
            std::size_t len = name.size();
            memcpy(write_buf_ptr->data(),&len,4);
            memcpy(write_buf_ptr->data()+4,name.data(),name.size());

            //构造参数, 参数一律用tuple包裹
            auto t_args = std::make_tuple(args...);
            msgpack::sbuffer ret = obj.pack(t_args); 
            len = ret.size();
            memcpy(write_buf_ptr->data()+4+name.size(),&len,4);
            memcpy(write_buf_ptr->data()+8+name.size(),ret.data(),ret.size());
            return 8+ret.size()+name.size();
    }

private:
    //网络相关
    std::string ip;
    unsigned short port;
    std::shared_ptr<boost::asio::io_service> service_ptr;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
    std::shared_ptr<boost::asio::io_service::work> work;
    std::shared_ptr<boost::asio::ip::tcp::endpoint> ep_ptr;
    
    //缓冲区
    std::shared_ptr<std::array<char, 2048>> write_buf_ptr;
    std::shared_ptr<std::array<char, 2048>> read_buf_ptr;

    //其他
    std::atomic_bool has_connected;
    std::shared_ptr<codec> code_ptr;;
};

#endif
