# network-library

由《Linux多线程服务端编程》改写的Reactor模式的多线程C++网络库,使用skiplist存储client数据，可在应用层添加其他功能。

目前的示例程序中使用protobuf序列化实现了简单的文件传送功能，client发送文件名，server返回结果与文件。

项目基本框架：

![image](https://github.com/nanjingu/network-library/blob/master/%E6%A1%86%E6%9E%B6.PNG)

运行示例：

![image](https://github.com/nanjingu/network-library/blob/master/example.PNG)
