#ifndef _RPC_CODEC_H
#define _RPC_CODEC_H

#include <msgpack.hpp>
#include <tuple>
#include <string>
#include "trait.h"

//编解码器
class codec final{
public:
    codec() = default;
    ~codec() = default;
    using buffer_type = msgpack::sbuffer;
    const int init_size = 2048;
    
    template <typename T> 
    buffer_type pack(T &&t){
        buffer_type buffer;
        msgpack::pack(buffer, std::forward<T>(t));
        return buffer;
    }

    template <typename... Args>
    buffer_type pack_args(const std::string funcName, Args &&...args) {
        buffer_type buffer(init_size);
        auto t = std::forward_as_tuple(args...);
        msgpack::pack(buffer, std::tuple<const std::string, decltype(t)>{funcName,t});
        return buffer;
    }

    template <typename Arg, typename... Args,typename = typename std::enable_if<std::is_enum<Arg>::value>::type>
    std::string pack_args_str(Arg arg, Args &&...args) {
        buffer_type buffer(init_size);
        msgpack::pack(buffer,
            std::forward_as_tuple((int)arg, std::forward<Args>(args)...));
        return std::string(buffer.data(), buffer.size());
    }
    
    //解出函数名
    std::tuple<std::string> unpack_func_name(const buffer_type& data){
        try{
            msgpack::unpack(msg_,data.data(),data.size());
            return msg_.get().as<std::tuple<std::string>>();
        }catch(...){
            std::cout << "函数名参数不匹配" << std::endl;
            return "";
        }
    }
    
    template<typename T>
    std::tuple<const std::string, T> unpack(const buffer_type& data){
        try{
            msgpack::unpack(msg_,data.data(),data.size());
            return msg_.get().as<std::tuple<const std::string, T>>();
        }catch(...){
            std::cout << "参数类型不匹配" << std::endl;
            return std::tuple<const std::string, T>();
        }        
    }

    template<typename T>
    T unpack(const char* data, const size_t length){
        try{
            msgpack::unpack(msg_,data,length);
            return msg_.get().as<T>();
        }catch(...){
            std::cout << "参数不匹配" << std::endl;
            return T();
        }
    }

    template<typename T>
    T unpack(const char* data, const size_t length, int& err){
        try{
            msgpack::unpack(msg_,data,length);
            err = 0;
            return msg_.get().as<T>();
        }catch(...){
            err = 1;
            return T();
        }
    }

public:
    msgpack::unpacked msg_;
};

#endif
