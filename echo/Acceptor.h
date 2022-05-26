#ifndef TINYEV_ACCEPTOR_H
#define TINYEV_ACCEPTOR_H

#include <memory>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Channel.h"

namespace ev
{

class EventLoop;

class Acceptor:noncopyable
{
public:
    Acceptor(EventLoop* loop, const InetAddress& local);
    ~Acceptor();

    bool listening() const
    { return listening_; }

    void listen();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

private:
    void handleRead();

    bool listening_;
    EventLoop* loop_;
    const int acceptFd_;
    Channel acceptChannel_;
    InetAddress local_;
    NewConnectionCallback newConnectionCallback_;
};

}



#endif //TINYEV_ACCEPTOR_H
