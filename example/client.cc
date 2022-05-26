#include <iostream>
#include <thread>

#include "../echo/Logger.h"
#include "../echo/TcpConnection.h"
#include "../echo/TcpClient.h"
#include "../echo/EventLoop.h"
#include "mytest.pb.h"

using namespace ev;

class UserInput: noncopyable
{
public:
    UserInput(const TcpConnectionPtr& conn):
            conn_(conn),
            logIn(false)
    {
        conn_->setMessageCallback(std::bind(
                &UserInput::onMessage, this, _1, _2));
    }


    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
    {
        if(!logIn){
            std::string line = buffer.retrieveAllAsString();
            std::cout<<line<<std::endl;
            logIn = true;
            return;
        }
        example::myResponse response;
        std::string line = buffer.retrieveAllAsString();
        if (!response.ParseFromString(line)) { 
            std::cout << "Failed to read msg." << std::endl; 
            return; 
        }
        std::cout << response.ans() << std::endl;
        std::cout << buffer.retrieveAllAsString() << std::endl;
    }

    void run()
    {
        std::cout<< "input username and passwd first:"<<std::endl;
        std::string username, passwd, data = "";
        std::cin>>username>>passwd;
        data += username += ' '; data += passwd;
        conn_->send(data);

        example::myRequest request;
        std::string line, opt;
        int num1 = 0, num2 = 0;
        while (1) {
            std::cin >> num1 >> opt >> num2;
            request.set_opt(opt.c_str());
            request.set_num1(num1);
            request.set_num2(num2);

            if (!request.SerializeToString(&line)) { 
                std::cout << "Failed to write msg." << std::endl; 
                return; 
            }
            conn_->send(line);
        }
        conn_->shutdown();
    }

private:
    TcpConnectionPtr conn_;
    bool logIn;
};

class EchoBench: noncopyable
{
public:
    EchoBench(EventLoop* loop, const InetAddress& addr):
            loop_(loop),
            client_(loop, addr)
    {
        client_.setConnectionCallback(std::bind(
                &EchoBench::onConnection, this, _1));
    }
    void start()
    { client_.start(); }

    void onConnection(const TcpConnectionPtr& conn)
    {
        INFO("connection %s is [%s]",
             conn->name().c_str(),
             conn->connected() ? "up" : "down");

        if (conn->connected()) {
            auto th = std::thread([conn](){
                UserInput user(conn);
                user.run();
            });
            th.detach();
        }
        else {
            loop_->quit();
        }
    }


private:
    EventLoop* loop_;
    TcpClient client_;
};

int main()
{
    setLogLevel(LOG_LEVEL_WARN);
    EventLoop loop;
    InetAddress addr("127.0.0.1", 8888);
    EchoBench client(&loop, addr);
    client.start();
    loop.loop();
}