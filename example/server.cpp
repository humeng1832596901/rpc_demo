#include <iostream>
#include <tuple>
#include "../include/rpc_server.h"
using namespace std;

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

class foo2{
public:
    void set_data(int d){
        data = d;
    }
    int get_data()const{
        return data;
    }
    MSGPACK_DEFINE(data);
private:
    int data;
};

int func(foo2 obj, int d){
    obj.set_data(d);
    return obj.get_data();
}

foo2 get_foo2(int n){
    foo2 obj;
    obj.set_data(n);
    return obj;
}

int main(int argc, char** argv){
    rpcServer* server = rpcServer::make_obj();
    server->init("../conf/test.conf");
    server->register_method("add",add);
    foo obj;
    server->register_method("sub",&foo::sub,&obj);
    server->register_method("div",foo::div);
    server->register_method("func",func);
    server->register_method("get_foo2",get_foo2);
    server->run();
    return 0;
}
