# rpc_demo

### 背景
`远程过程调用(rpc)`是一种重要的IPC方式之一，在微服务中具有重要的应用价值。

在前辈们发明出适用不同场景的I/O多路复用模型后，服务系统的性能得到了提升，但随着业务场景的不断升级，网络流量的飞速增加，一些服务系统的稳定性或者效率遭受到了一定挑战，
于是人们就又开始探索压榨系统性能的方法了。一个服务系统中，存在着不同过程调用分别处理不同的任务，有I/O密集型的任务，也有CPU密集型的任务，如果我们把这些过程分散在不同的主机中(`分布式`)，对处理CPU密集型过程的主机进行`集群`，便可更进一步提高系统性能。不过，这样子就会面临一个问题，传统的IPC方式将不再适用，因为这些过程都不在同一个主机中，于是就有了rpc。

### 使用
由于代码量较小，所有文件均为.h头文件，将源代码包含进源程序即可(更多详见example)，网络通信部分依赖`asio库`，序列化和反序列化使用`msgpack`。c++版本要求>=14。

server
```c++
#include "../include/rpc_server.h"

int add(int a, int b){
    return a+b;
}

class foo{
public:
    int sub(int a, int b){
        return a-b;
    }
    static double div(double a, double b){
        return a/b;
    }
};

int main(int argc, char** argv){
    rpcServer* server = rpcServer::make_obj();
    server->init("../conf/test.conf");
    server->register_method("add",add);
    foo obj;
    server->register_method("sub",&foo::sub,&obj);
    server->register_method("div",foo::div);
    server->run();
    return 0;
}
```

client
```c++
#include <iostream>
#include <string>
#include "../include/rpc_client.h"

int main(int argc, char** argv){
    auto client = std::make_shared<rpcClient>("127.0.0.1",8080);
    client->connect();
    try{
        int res = client->call<int>(std::string("add"),1,2);
        std::cout << res << std::endl;

        double res2 = client->call<double>("div",1,2);
        std::cout << res2 << std::endl;
        
    }catch(std::string err){
        std::cerr << err << std::endl;
    }
    return 0;
}
```

### 代码统计
![在这里插入图片描述](https://img-blog.csdnimg.cn/8c2d1bf995af428b94d79f5fc1117ef4.jpg?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBAcXFfMjAwMTQ2MTE=,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

### 其他代码细节
[知乎文章](https://zhuanlan.zhihu.com/p/419860500)

### To-do List
- [x] 函数注册和调用
  - [x] 普通函数
  - [x] 类成员函数
  - [x] 类静态函数
- [x] 简单同步通信
- [ ] 函数注册和调用
  - [ ] 重载普通函数
  - [ ] 普通模板函数
- [ ] 异步多线程(io_service池)
- [ ] 长连接
- [ ] ssl加密传输



![GitHub stars](https://img.shields.io/badge/%E7%9D%A1%E5%A4%A7%E8%A7%89-%E8%BA%AB%E4%BD%93%E5%81%A5%E5%BA%B7-green) ![GitHub stars](https://img.shields.io/badge/%E9%A6%99%E6%A6%AD%E4%B8%BD%E8%88%8D-%E6%85%A2%E6%85%A2%E8%B5%B0-brightgreen) ![GitHub stars](https://img.shields.io/badge/%E7%BF%A1%E5%86%B7%E7%BF%A0-%E6%97%85%E8%A1%8C-orange)
