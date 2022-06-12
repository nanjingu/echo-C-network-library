#include <iostream>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
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
            printf("%s\n", line.c_str());
            logIn = true;
            flag = true;
            return;
        }
        example::myResponse response;
        std::string line = buffer.retrieveAllAsString();
        if (!response.ParseFromString(line)) { 
            INFO("Failed to read msg."); 
            return; 
        }
        int resCode = response.ans();
        std::string res = response.file();
        if(resCode != 200){
            INFO("download fail.code is %d and result is %s", resCode, res);
            return;
        }
        else{
            printf("download file...\n");
            int filefd = open(saveName.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(filefd < 0){
                INFO("create %s error\n");
            }

            else{
                write(filefd, res.c_str(), sizeof(res));
                printf("download done and save as %s\nnow you can download other file.\n", saveName.c_str());
            }
        }
        flag = true;
    }

    void run()
    {
        printf("input username and passwd: \n");
        std::string username, passwd, data = "";
        std::cin>>username>>passwd;
        data += username += ' '; data += passwd;
        conn_->send(data);

        example::myRequest request;
        std::string line, url;
        // while(flag == false);
        // flag = false;
        while (1) {
            std::cin >> url >> saveName;
            request.set_url(url.c_str());

            if (!request.SerializeToString(&line)) { 
                INFO("Failed to write msg."); 
                return; 
            }
            conn_->send(line);
            while(flag == false);
            flag = false;
        }
        conn_->shutdown();
    }

private:
    TcpConnectionPtr conn_;
    bool logIn;
    static std::string saveName;
    static bool flag;
};
std::string UserInput::saveName = "";
bool UserInput::flag = false;

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
