#include <map>

#include "../echo/Logger.h"
#include "../echo/EventLoop.h"
#include "../echo/TcpConnection.h"
#include "../echo/TcpServer.h"
#include "mytest.pb.h"
#include "../echo/skiplist.h"

using namespace ev;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, size_t numThread = 1, Nanosecond timeout = 5s)
            : username(""),
              passwd(""),
              skiplist(6),
              loop_(loop),
              server_(loop, addr),
              numThread_(numThread),
              timeout_(timeout),
              timer_(loop_->runEvery(timeout_, [this](){onTimeout();}))
    {
        skiplist.download_data();
        server_.setConnectionCallback(std::bind(
                &EchoServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(
                &EchoServer::onMessage, this, _1, _2));
        server_.setWriteCompleteCallback(std::bind(
                &EchoServer::onWriteComplete, this, _1));
    }

    ~EchoServer()
    { loop_->cancelTimer(timer_); }

    void start()
    {
        // set thread num here
        server_.setNumThread(numThread_);
        server_.start();
    }

    void onConnection(const TcpConnectionPtr& conn)
    {
        INFO("connection %s is [%s]",
             conn->name().c_str(),
             conn->connected() ? "up":"down");

        if (conn->connected()) {
            conn->setHighWaterMarkCallback(
                    std::bind(&EchoServer::onHighWaterMark, this, _1, _2),
                    1024);
            expireAfter(conn, timeout_);
        }
        else connections_.erase(conn);
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer& buffer)
    {
        TRACE("connection %s recv %lu bytes",
             conn->name().c_str(),
             buffer.readableBytes());

        // send will retrieve the buffer
        if(username.empty()){ 
            std::string n, p;
            std::string line = buffer.retrieveAllAsString();
            int i = 0, m = line.size();
            while(i < m && line[i] != ' ')n += line[i++];
            ++i;
            while(i < m)p += line[i++];
            username = n; 
            passwd = p;
            std::cout<<"username: "<<username<<" passwd: "<<passwd<<std::endl;
            skiplist.insert_element(username, passwd);
            skiplist.upload_data();
            line = "input your data";
            conn->send(line);
            return;
        }
        example::myRequest request;
        std::string line = buffer.retrieveAllAsString();
        if (!request.ParseFromString(line)) { 
            std::cout << "Failed to read msg." << std::endl; 
            return; 
        }
        char opt = request.opt()[0];
        ::google::protobuf::int32 ans = 0, num1 = request.num1(), num2 = request.num2();
        switch(opt){
            case '+' : ans = num1 + num2;break;
            case '-' : ans = num1 - num2;break;
            case '*' : ans = num1 * num2;break;
            case '/' : ans = num1 / num2;break;
        }
        example::myResponse response;
        response.set_ans(ans);
        if (!response.SerializeToString(&line)) { 
            INFO("Failed to read msg.");
            return; 
        }
        INFO("receive %d %c %d, send %d", num1, opt, num2, ans);
        conn->send(line);
        expireAfter(conn, timeout_);
    }

    void onHighWaterMark(const TcpConnectionPtr& conn, size_t mark)
    {
        INFO("high water mark %lu bytes, stop read", mark);
        conn->stopRead();
        expireAfter(conn, 2 * timeout_);
    }

    void onWriteComplete(const TcpConnectionPtr& conn)
    {
        if (!conn->isReading()) {
            INFO("write complete, start read");
            conn->startRead();
            expireAfter(conn, timeout_);
        }
    }

private:
    void expireAfter(const TcpConnectionPtr& conn, Nanosecond interval)
    {
        connections_[conn] = clock::nowAfter(interval);
    }

    void onTimeout()
    {
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            if (it->second <= clock::now()) {
                INFO("connection timeout force close");
                it->first->forceClose();
                it = connections_.erase(it);
            }
            else it++;
        }
    }

private:
    std::string username;
    std::string passwd;
    SkipList<std::string, std::string> skiplist;
    EventLoop* loop_;
    TcpServer server_;
    const size_t numThread_;
    const Nanosecond timeout_;
    Timer* timer_;
    typedef std::map<TcpConnectionPtr, Timestamp> ConnectionList;
    ConnectionList connections_;
};

int main()
{
    setLogLevel(LOG_LEVEL_TRACE);
    EventLoop loop;
    InetAddress addr(8888);
    EchoServer server(&loop, addr, 1, 50s);
    server.start();

    loop.loop();
}