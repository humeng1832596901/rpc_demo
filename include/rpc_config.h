/*******************************************************************
	> File Name: rpc_config.h
	> Author: 胡孟
	> Mail: 13535324513@163.com
	> Created Time: Sun 26 Sep 2021 12:51:14 PM CST
 ******************************************************************/

#ifndef _RPC_CONFIG_H
#define _RPC_CONFIG_H

#include<string>
#include<unordered_map>
#include<fstream>
#include<iostream>

//读取配置文件类
class rpcConfig final {
public:
    rpcConfig() = default;

    //读入配置文件
    void load_config_file(const char* path){
        std::ifstream in(path);
        if(!in.is_open()){
            std::cerr << "rpcConfig::LoadConfigFile: open error" << std::endl;
            return;
        }
        char buf[1024];
        while(!in.eof()){
            in.getline(buf,sizeof(buf));
            if(buf[0]!='\0')
                this->parse(buf);
        }
    }

    //解析各个变量
    void parse(std::string str){
        unsigned long notSpace = 0;
        while(notSpace<str.size() &&str[notSpace]==' ') notSpace++;
        //空行或注释
        if(notSpace==str.size() || str[notSpace]=='#')
            return;

        std::string key, value;

        //提取key
        int idx = str.find('=',notSpace);
        if(idx==-1)
            return;

        //去除等号前面空格
        int end = idx-1;
        while(end>=(int)notSpace && str[end]==' ') end--;
        key = std::string(str.begin()+notSpace, str.begin()+end+1);

    
        //提取value
        notSpace = idx+1;
        while(notSpace<str.size() && str[notSpace]==' ') notSpace++;
        value = std::string(str.begin()+notSpace, str.end());
        paras.insert(std::pair<std::string, std::string>{key,value});
    }

    //加载配置文件中的信息
    std::string load(const std::string& key){    
        if(paras.find(key)==paras.end())
            return "";
        return paras[key];
    }

    void show(){    
    for(auto& elem : paras)
        std::cout << elem.first << " : " << elem.second << std::endl; 
    }

private:
    //各个参数键值对
    std::unordered_map<std::string, std::string> paras;
    
};
#endif
