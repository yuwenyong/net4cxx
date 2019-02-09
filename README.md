
# net4cxx

## Requirements

### Windows（以mingw64(msys2)为例）

* 运行以下命令安装必备的工具链和模块

```bash
pacman-key --init
pacman -Syu
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-extra-cmake-modules
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-gdb
pacman -S mingw-w64-x86_64-toolchain

pacboy update
pacboy sync msys/git msys/make boost:x
pacboy sync dlfcn:x
```

* 下载libbacktrace源码编译安装

```bash
./configure
./make
./make install
```

* 接下来即可编译本模块

### linux（以ubuntu为例子）

* 需要1.66以上的boost,如果是18.10以上版本的ubuntu,则可以直接通过命令安装，否则源码编译boost

```bash
sudo apt-get update
sudo apt-get install git cmake make gcc g++
sudo apt-get install zlib1g-dev
sudo apt-get install libssl-dev
sudo apt-get install libboost-all-dev
```

### macOS

* 需要先安装Xcode
* 源码编译安装zlib
* 源码编译安装cmake, openssl和boost,或运行以下命令

```bash
brew update
brew install openssl cmake boost
```

## Build

* 运行以下命令

```bash
cd net4cxx
mkdir build
cd build
cmake ..
make
```

## Tutorial

### 开发基于字节流协议的服务器(tcp,ssl,unix)


```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class Echo: public Protocol, public std::enable_shared_from_this<Echo> {
public:
    void dataReceived(Byte *data, size_t length) override {
        write(data, length);
    }
};

class EchoFactory: public Factory {
public:
    ProtocolPtr buildProtocol(const Address &address) override {
        return std::make_shared<Echo>();
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    TCPServerEndpoint endpoint(&reactor, "28001");
    endpoint.listen(std::make_shared<EchoFactory>());
    reactor.run();
    return 0;
}

```

* 以上实现了一个最简单的例子,将客户端发送过来的信息回写;
* 将TCPServerEndPoint替换成SSLServerEndPoint将会启动一个基于sslSocket的服务器;
* 将TCPServerEndPoint替换成UNIXServerEndPoint将会启动一个基于unixSocket的服务器;
* 后面会展示一种更好的服务器启动方式,切换协议无需修改任何代码.

```c++

class Echo: public Protocol, public std::enable_shared_from_this<Echo> {
public:
    void connectionMade() override {
        NET4CXX_LOG_INFO(gAppLog, "Connection made");
    }
    
    void connectionLost(std::exception_ptr reason) override {
        NET4CXX_LOG_INFO(gAppLog, "Connection lost");
    }

    void dataReceived(Byte *data, size_t length) override {
        write(data, length);
        loseConnection();
    }
};

```

* 连接建立时将会回调connectionMade;
* 连接销毁时将会回调connectionLost;
* 调用loseConnection安全的关闭连接.


```c++

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    serverFromString(&reactor, "tcp:28001")->listen(std::make_shared<EchoFactory>());
    serverFromString(&reactor, "ssl:28002:privateKey=test.key:certKey=test.crt")->listen(std::make_shared<EchoFactory>());
    serverFromString(&reactor, "unix:/var/foo/bar")->listen(std::make_shared<EchoFactory>());
    reactor.run();
    return 0;
}

```

* 以上代码展示了在一个进程内同时启动了一个tcp服务器,ssl服务器,unix服务器;
* 观察服务器的启动方式,发现只有字符串参数的值不同,如果我们从配置中读取这个字符串的话,切换协议无须更改一行代码,这也是推荐的方式;

### 开发基于字节流协议的客户端(tcp,ssl,unix)

```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class WelcomeMessage: public Protocol, public std::enable_shared_from_this<WelcomeMessage> {
public:
    void connectionMade() override {
        write("Hello server, I am the client!");
        loseConnection();
    }
};

class WelcomeFactory: public ClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        return std::make_shared<WelcomeMessage>();
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectTCP("localhost", "28001", std::make_shared<WelcomeFactory>());
    reactor.run();
    return 0;
}

```

* 以上实现了一个最简单的例子,客户端向服务器打了个招乎,随后关闭连接;
* 将connectTCP替换成connectSSL或者connectUNIX能分别建立ssl或者unix客户端连接;
* 与服务器一样支持从字符串构建客户端连接,调用clientFromString即可,不再赘述;


```c++
int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectTCP("localhost", "28001", std::make_shared<OneShotFactory>(std::make_shared<WelcomeMessage>()));
    reactor.run();
    return 0;
}
```

* 使用内置的OneShotFactory可以指定总是返回某个固定的protocol,用户也无需创建自己的factory,这在服务器之间互联很有用;

```c++

class WelcomeFactory: public ReconnectingClientFactory {
public:
    std::shared_ptr<Protocol> buildProtocol(const Address &address) override {
        resetDelay();
        return std::make_shared<WelcomeMessage>();
    }
};

```

* 当继承自内建的ReconnectingClientFactory,连接断开时,会自动启用指数避让原则进行重连;

### 开发基于数据报协议的服务器

```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class Echo: public DatagramProtocol, public std::enable_shared_from_this<Echo> {
public:
    void datagramReceived(Byte *datagram, size_t length, Address address) override {
        std::string s((char *)datagram, (char *)datagram + length);
        NET4CXX_LOG_INFO(gAppLog, "Datagram received: %s From %s:%u", s.c_str(), address.getAddress().c_str(),
                         address.getPort());
        write(datagram, length, address);
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.listenUDP(28002, std::make_shared<Echo>());
    reactor.run();
    return 0;
}
```

* 以上实现了一个最简单的echo服务器;
* 将listenUDP调用替换成listenUNIXDatagram可以使用unix域数据报套接字;

### 开发基于数据报协议的客户端

```c++
#include "net4cxx/net4cxx.h"

using namespace net4cxx;

class EchoClient: public DatagramProtocol, public std::enable_shared_from_this<EchoClient> {
public:
    void startProtocol() override {
        write("Hello boy!");
    }

    void datagramReceived(Byte *datagram, size_t length, Address address) override {
        std::string s((char *)datagram, (char *)datagram + length);
        NET4CXX_LOG_INFO(gAppLog, "Datagram received: %s From %s:%u", s.c_str(), address.getAddress().c_str(),
                         address.getPort());
    }
};

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    reactor.connectUDP("127.0.0.1", 28002, std::make_shared<MyProtocol>());
    reactor.connectUNIXDatagram("/data/foo/bar", std::make_shared<MyProtocol>(), 8192, "/data/foo/bar2");
    reactor.run();
    return 0;
}
```

* 以上实现了一个最简单的udp client;
* 将connectUDP调用替换成connectUNIXDatagram可以使用unix域数据报套接字;

### 开发基于websocket的服务端

```c++
#include "net4cxx/net4cxx.h"


using namespace net4cxx;

class BroadcastServerFactory: public WebSocketServerFactory,
                              public std::enable_shared_from_this<BroadcastServerFactory> {
public:
    using WebSocketServerFactory::WebSocketServerFactory;

    ProtocolPtr buildProtocol(const Address &address) override;

    void registerClient(WebSocketServerProtocolPtr client) {
        if (_clients.find(client) == _clients.end()) {
            NET4CXX_LOG_INFO(gAppLog, "register client %s", client->getPeerName());
            _clients.insert(client);
        }
    }

    void unregisterClient(WebSocketServerProtocolPtr client) {
        if (_clients.find(client) != _clients.end()) {
            NET4CXX_LOG_INFO(gAppLog, "unregister client %s", client->getPeerName());
            _clients.erase(client);
        }
    }

    void broadcast(const std::string &msg) {
        NET4CXX_LOG_INFO(gAppLog, "broacasting message '%s' ..", msg);
        for (auto c: _clients) {
            c->sendMessage(msg);
            NET4CXX_LOG_INFO(gAppLog, "message sent to %s", c->getPeerName());
        }
    }
protected:
    std::set<WebSocketServerProtocolPtr> _clients;
};


class BroadcastServerProtocol: public WebSocketServerProtocol {
public:
    void onOpen() override {
        getFactory<BroadcastServerFactory>()->registerClient(getSelf<BroadcastServerProtocol>());
    }

    void onMessage(ByteArray payload, bool isBinary) override {
        if (!isBinary) {
            auto msg = StrUtil::format("%s from %s", BytesToString(payload), getPeerName());
            getFactory<BroadcastServerFactory>()->broadcast(msg);
        }
    }

    void connectionLost(std::exception_ptr reason) override {
        WebSocketServerProtocol::connectionLost(reason);
        getFactory<BroadcastServerFactory>()->unregisterClient(getSelf<BroadcastServerProtocol>());
    }
};

ProtocolPtr BroadcastServerFactory::buildProtocol(const Address &address) {
    return std::make_shared<BroadcastServerProtocol>();
}

int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    auto factory = std::make_shared<BroadcastServerFactory>("ws://127.0.0.1:9000");
    listenWS(&reactor, factory);
    reactor.run();
    return 0;
}
```

* 以上实现了一个具有广播功能的websocket服务器;

### 开发基于websocket的客户端

```c++
#include "net4cxx/net4cxx.h"


using namespace net4cxx;


class BroadcastClientProtocol: public WebSocketClientProtocol {
public:
    void onOpen() override {
        sendHello();
    }

    void onMessage(ByteArray payload, bool isBinary) override {
        if (!isBinary) {
            NET4CXX_LOG_INFO(gAppLog, "Text message received: %s", BytesToString(payload));
        }
    }

    void sendHello() {
        sendMessage("Hello from client!");
        reactor()->callLater(2.0, [this, self=shared_from_this()](){
            sendHello();
        });
    }
};


class BroadcastClientFactory: public WebSocketClientFactory {
public:
    using WebSocketClientFactory::WebSocketClientFactory;

    ProtocolPtr buildProtocol(const Address &address) override {
        return std::make_shared<BroadcastClientProtocol>();
    }
};


int main(int argc, char **argv) {
    NET4CXX_PARSE_COMMAND_LINE(argc, argv);
    Reactor reactor;
    auto factory = std::make_shared<BroadcastClientFactory>("ws://127.0.0.1:9000");
    connectWS(&reactor, factory);
    reactor.run();
    return 0;
}
```

* 以上实现了一个会自动定时发消息的websocket客户端

## Reference

### 核心模块

#### Reactor

```c++
class Reactor;
```

##### 默认构造函数

```c++
Reactor();
```

##### 启动反应器循环

```c++
void run(bool installSignalHandlers=true);
```

* installSignalHandlers： 是否安装信号退出处理器

##### 定时调用某个函数

```c++
template <typename CallbackT>
DelayedCall callLater(double deadline, CallbackT &&callback);

template <typename CallbackT>
DelayedCall callLater(const Duration &deadline, CallbackT &&callback);
```

* deadline: 延迟多少秒触发
* callback: 回调函数，满足签名void ()

```c++
template <typename CallbackT>
DelayedCall callAt(time_t deadline, CallbackT &&callback);

template <typename CallbackT>
DelayedCall callAt(const Timestamp &deadline, CallbackT &&callback);
```

* deadline: 触发的绝对时间
* callback: 回调函数，满足签名void ()

##### 在服务器的下一帧触发回调

```c++
template <typename CallbackT>
void addCallback(CallbackT &&callback);
```

* callback: 回调函数，满足签名void ()

##### 添加一个反应器退出时的回调函数

```c++
template <typename CallbackT>
void addStopCallback(CallbackT &&callback);
```

* callback: 回调函数，满足签名void ()

##### 启动一个tcp服务器

```c++
ListenerPtr listenTCP(const std::string &port, std::shared_ptr<Factory> factory, const std::string &interface={});
```

* port: 端口号或服务名称
* factory： 协议工厂
* interface: 指定监听的ip地址或域名

##### 启动一个tcp客户端

```c++
ConnectorPtr connectTCP(const std::string &host, const std::string &port, std::shared_ptr<ClientFactory> factory,
                        double timeout=30.0, const Address &bindAddress={});
```

* host: 连接的服务器的ip地址或域名
* port: 连接的服务器的端口或服务名
* factory： 协议工厂
* timeout： 连接超时时间
* bindAddress: 为客户端套接字绑定一个指定的地址和端口

##### 启动一个ssl服务器

```c++
ListenerPtr listenSSL(const std::string &port, std::shared_ptr<Factory> factory, SSLOptionPtr sslOption,
                      const std::string &interface={});
```

* port: 端口号或服务名称
* factory： 协议工厂
* sslOption: ssl选项
* interface: 指定监听的ip地址或域名

##### 启动一个ssl客户端

```c++
ConnectorPtr connectSSL(const std::string &host, const std::string &port, std::shared_ptr<ClientFactory> factory,
                        SSLOptionPtr sslOption, double timeout=30.0, const Address &bindAddress={});
```

* host: 连接的服务器的ip地址或域名
* port: 连接的服务器的端口或服务名
* factory： 协议工厂
* sslOption: ssl选项
* timeout： 连接超时时间
* bindAddress: 为客户端套接字绑定一个指定的地址和端口

##### 启动一个udp服务器

```c++
DatagramConnectionPtr listenUDP(unsigned short port, DatagramProtocolPtr protocol, const std::string &interface="",
                                size_t maxPacketSize=8192, bool listenMultiple=false);
```

* port: 绑定的端口号
* protocol: 协议处理器
* interfance: 绑定的ip地址
* maxPacketSize: 接收数据报的最大尺寸
* listenMultiple: 允许多个套接字绑定相同的地址

##### 启动一个udp客户端

```c++
DatagramConnectionPtr connectUDP(const std::string &address, unsigned short port, DatagramProtocolPtr protocol,
                                 size_t maxPacketSize=8192, const Address &bindAddress={},
                                 bool listenMultiple=false);
```

* address: 连接的ip地址
* port: 连接的端口号
* protocol: 协议处理器
* interfance: 绑定的ip地址
* maxPacketSize: 接收数据报的最大尺寸
* bindAddress: 为客户端套接字绑定一个指定的地址和端口
* listenMultiple: 允许多个套接字绑定相同的地址

##### 启动一个unix服务器

```c++
ListenerPtr listenUNIX(const std::string &path, std::shared_ptr<Factory> factory);
```

* path: 监听的文件路经
* factory： 协议工厂

##### 启动一个unix客户端

```c++
ConnectorPtr connectUNIX(const std::string &path, std::shared_ptr<ClientFactory> factory, double timeout=30.0);
```

* path: 服务器的文件路经
* factory： 协议工厂
* timeout： 连接超时时间

##### 启动一个unix域数据报服务器

```c++
DatagramConnectionPtr listenUNIXDatagram(const std::string &path, DatagramProtocolPtr protocol,
                                         size_t maxPacketSize=8192);
```

* path: 绑定的文件路经
* protocol: 协议处理器
* maxPacketSize: 接收数据报的最大尺寸

##### 启动一个unix域数据报客户端

```c++
DatagramConnectionPtr connectUNIXDatagram(const std::string &path, DatagramProtocolPtr protocol,
                                          size_t maxPacketSize=8192, const std::string &bindPath="");
```

* path: 连接的服务器的文件路经
* protocol: 协议处理器
* maxPacketSize: 接收数据报的最大尺寸
* bindPath: 绑定的文件路经(如果要接收数据,必须绑定一个路经)


##### 获取reactor是否正在运行

```c++
bool running() const;
```

##### 域名解析

```c++
template <typename CallbackT>
DelayedResolve resolve(const std::string &name, CallbackT &&callback);
```

* name: 要解析的域名
* callback: 回调函数，满足签名void (StringVector)

##### 停止reactor

```c++
void stop();
```

#### DelayedCall

```
class DelayedCall;
```

##### 获取计时器是否过期

```c++
bool cancelled() const;
```

##### 取消该计时器

```c++
void cancel();
```

#### DelayedResolve

```c++
class DelayedResolve;
```

##### 获取域名解析是否已经结束

```c++
bool cancelled() const;
```

##### 取消本次域名解析

```c++
void cancel();
```

#### Address

```c++
class Address;
```

##### 构造函数

```c++
explicit Address(std::string address="", unsigned short port=0);
```

* address: ip地址或文件路经(unix)
* port: 端口号, 对unix域地址无效

##### 设置地址

```c++
void setAddress(std::string &&address);
void setAddress(const std::string &address);
```

* address: ip地址或文件路经(unix)

##### 获取地址

```c++
const std::string& getAddress() const
```

##### 设置端口号

```c++
void setPort(unsigned short port);
```

* port: 端口号, 对unix域地址无效

##### 获取端口号

```c++
unsigned short getPort() const;
```

##### 操作符和类型重载

```c++
explicit operator bool() const;
bool operator!() const;
```

#### 全局函数

##### 判断两个地址相等性

```c++
bool operator==(const Address &lhs, const Address &rhs);
bool operator!=(const Address &lhs, const Address &rhs);
```

### SSL工具

#### SSLVerifyMode

```c++
enum class SSLVerifyMode {
    CERT_NONE,
    CERT_OPTIONAL,
    CERT_REQUIRED,
}
```

* CERT_NONE: 不启用认证
* CERT_OPTIONAL: 启用认证
* CERT_REQUIRED: 启用认证，对端没有证书时认证失败

#### SSLParams

```c++
class SSLParams;
```

##### 构造函数

```c++
explicit SSLParams(bool serverSide= false);
```

* serverSide: 是否用于服务端

##### 设置证书文件

```c++
void setCertFile(const std::string &certFile);
```

* certFile: 证书文件名

##### 获取证书文件名

```c++
const std::string& getCertFile() const;
```

##### 设置私钥文件名

```c++
void setKeyFile(const std::string &keyFile);
```

* keyFile: 私钥文件名

##### 获取私钥文件名

```c++
const std::string& getKeyFile() const;
```

##### 设置密码

```c++
void setPassword(const std::string &password);
```

* password: 密码

##### 获取密码

```c++
const std::string& getPassword() const;
```

##### 设置验证模式

```c++
void setVerifyMode(SSLVerifyMode verifyMode);
```

* verifyMode: 验证模式

##### 获取验证模式

```c++
SSLVerifyMode getVerifyMode() const;
```

##### 设置CA文件

```c++
void setVerifyFile(const std::string &verifyFile);
```

* verifyFile: CA文件名

##### 获取CA文件

```c++
const std::string& getVerifyFile() const;
```

##### 设置要验证的主机名

```c++
void setCheckHost(const std::string &hostName);
```

* hostname: 主机名

##### 获取要验证的主机名

```c++
const std::string& getCheckHost() const;
```

##### 获取是否用于服务端

```c++
bool isServerSide() const;
```

#### SSLOption

```c++
class SSLOption;
```

##### 获取是否用于服务端

```c++
bool isServerSide() const;
```

##### 创建SSLOption

```c++
static SSLOptionPtr create(const SSLParams &sslParams);
```

* sslParams: 用于初始化sslContext的参数


### 基础流协议模块

#### Factory

```c++
class Factory;
```

##### 开始时回调

```c++
virtual void startFactory();
```

##### 结束时回调

```c++
virtual void stopFactory();
```

##### 接收到新连接时回调

```c++
virtual ProtocolPtr buildProtocol(const Address &address) = 0;
```

#### ClientFactory

```c++
class ClientFactory: public Factory;
```

##### 开始连接时回调

```c++
virtual void startedConnecting(ConnectorPtr connector);
```

##### 连接失败时回调

```c++
virtual void clientConnectionFailed(ConnectorPtr connector, std::exception_ptr reason);
```

##### 连接断开时回调

```c++
virtual void clientConnectionLost(ConnectorPtr connector, std::exception_ptr reason);
```

#### OneShotFactory

```c++
class OneShotFactory: public ClientFactory;
```

##### 构造函数

```c++
explicit OneShotFactory(ProtocolPtr protocol);
```

* protocol: 绑定指定的protocol对象

#### ReconnectingClientFactory

```c++
class ReconnectingClientFactory: public ClientFactory;
```

##### 停止连接

```c++
void stopTrying();
```

##### 获取最大的延迟时间

```c++
double getMaxDelay() const
```

##### 设置最大的延迟时间

```c++
void setMaxDelay(double maxDelay);
```

* maxDelay: 最大的重试延迟时间

##### 获取最大的重试次数

```c++
int getMaxRetires() const;
```

##### 设置最大的重试次数

```c++
void setMaxRetries(int maxRetries);
```

* maxRetries: 最大的重试次数

##### 重置重连延迟和次数信息

```c++
void resetDelay();
```

#### Protocol

```c++
class Protocol;
```

##### 连接建立时回调

```c++
virtual void connectionMade();
```

##### 收到数据时回调

```c++
virtual void dataReceived(Byte *data, size_t length) = 0;
```

* data: 数据包
* length: 数据包长度

##### 连接关闭时回调

```c++
virtual void connectionLost(std::exception_ptr reason);
```

* reason: 关闭原因

##### 获取关联的反应器

```c++
Reactor* reactor();
```

##### 发送数据

```c++
void write(const Byte *data, size_t length);
void write(const ByteArray &data);
void write(const char *data);
void write(const std::string &data);
```

* data: 数据包
* length: 数据包长度

##### 安全的关闭连接

```c++
void loseConnection();
```

##### 快速的关闭连接

```c++
void abortConnection();
```

##### 获取nodelay选项

```c++
bool getNoDelay() const;
```

##### 设置nodelay选项

```c++
void setNoDelay(bool enabled);
```

* enabled: 是否启用nodelay

##### 获取keepalive选项

```c++
bool getKeepAlive() const;
```

##### 设置keepalive选项

```c++
void setKeepAlive(bool enabled);
```

* enabled: 是否启用keepalive

##### 获取本地地址

```c++
std::string getLocalAddress() const;
```

##### 获取本地端口

```c++
unsigned short getLocalPort() const;
```

##### 获取对端地址

```c++
std::string getRemoteAddress() const;
```

##### 获取对端端口

```c++
unsigned short getRemotePort() const;
```

##### 获取关联的Factory

```c++
template <typename FactoryT>
std::shared_ptr<FactoryT> getFactory() const;
```

#### Listener

```c++
class Listener;
```

##### 开始监听

```c++
virtual void startListening()=0;
```

##### 停止监听

```c++
virtual void stopListening()=0;
```

##### 获取关联的反应器

```c++
Reactor* reactor();
```

#### Connector

```c++
class Connector;
```

##### 开始连接

```c++
virtual void startConnecting()=0;
```

#### 停止连接

```c++
virtual void stopConnecting()=0;
```

##### 获取关联的反应器

```c++
Reactor* reactor();
```

#### Endpoint

```c++
class Endpoint;
```

##### 构造函数

```c++
explicit Endpoint(Reactor *reactor);
```

* 关联的反应器

##### 获取关联的反应器

```c++
Reactor* reactor();
```

#### ServerEndpoint

```c++
class ServerEndpoint: public Endpoint;
```

##### 开始监听

```c++
virtual ListenerPtr listen(std::shared_ptr<Factory> protocolFactory) const = 0;
```

* protocolFactory: 协议工厂

#### TCPServerEndpoint

```c++
class TCPServerEndpoint: public ServerEndpoint;
```

##### 构造函数

```c++
TCPServerEndpoint(Reactor *reactor, std::string port, std::string interface={});
```

* reactor: 关联的反应器
* port: 绑定的端口
* interface: 绑定的地址

#### SSLServerEndpoint

```c++
class SSLServerEndpoint: public ServerEndpoint;
```

##### 构造函数

```c++
SSLServerEndpoint(Reactor *reactor, std::string port, SSLOptionPtr sslOption, std::string interface={});
```

* reactor: 关联的反应器
* sslOption: ssl选项
* port: 绑定的端口
* interface: 绑定的地址

#### UNIXServerEndpoint

```c++
class UNIXServerEndpoint: public ServerEndpoint;
```

##### 构造函数

```c++
UNIXServerEndpoint(Reactor *reactor, std::string path);
```

* reactor: 关联的反应器
* path: 绑定的路径

#### ClientEndpoint

```c++
class ClientEndpoint: public Endpoint;
```

##### 开始连接

```c++
virtual ConnectorPtr connect(std::shared_ptr<ClientFactory> protocolFactory) const = 0;
```

* protocolFactory: 协议工厂

#### TCPClientEndpoint

```c++
class TCPClientEndpoint: public ClientEndpoint;
```

##### 构造函数

```c++
TCPClientEndpoint(Reactor *reactor, std::string host, std::string port, double timeout=30.0, Address bindAddress={});
```

* reactor: 关联的反应器
* host: 要连接的对端地址
* port: 要连接的对端端口
* timeout: 连接超时时间
* bindAddress: 绑定的本地地址

#### SSLClientEndpoint

```c++
class SSLClientEndpoint: public ClientEndpoint;
```

##### 构造函数

```c++
SSLClientEndpoint(Reactor *reactor, std::string host, std::string port, SSLOptionPtr sslOption, double timeout=30.0, Address bindAddress={});
```

* reactor: 关联的反应器
* host: 要连接的对端地址
* port: 要连接的对端端口
* sslOption: ssl上下文
* timeout: 连接超时时间
* bindAddress: 绑定的本地地址

#### UNIXClientEndpoint

```c++
class UNIXClientEndpoint: public ClientEndpoint;
```

##### 构造函数

```c++
UNIXClientEndpoint(Reactor *reactor, std::string path, double timeout=30.0);
```

* reactor: 关联的反应器
* path: 要连接的对端路经
* timeout: 连接超时时间

#### 全局函数

##### 启动服务器

```c++
///
/// \param reactor
/// \param description
///     tcp:80
///     tcp:80:interface=127.0.0.1
///     ssl:443:privateKey=key.pem:certKey=crt.pem
///     unix:/var/run/finger
/// \return
ServerEndpointPtr serverFromString(Reactor *reactor, const std::string &description);
```

##### 连接客户端

```c++
///
/// \param reactor
/// \param description
///     tcp:host=www.example.com:port=80
///     tcp:www.example.com:80
///     tcp:host=www.example.com:80
///     tcp:www.example.com:port=80
///     ssl:web.example.com:443:privateKey=foo.pem:certKey=foo.pem
///     ssl:host=web.example.com:port=443:caCertsDir=/etc/ssl/certs
///     tcp:www.example.com:80:bindAddress=192.0.2.100
///     unix:path=/var/foo/bar:timeout=9
///     unix:/var/foo/bar
///     unix:/var/foo/bar:timeout=9
/// \return
ClientEndpointPtr clientFromString(Reactor *reactor, const std::string &description);
```

##### 在指定的协议处理器上连接客户端

```c++
ConnectorPtr connectProtocol(const ClientEndpoint &endpoint, ProtocolPtr protocol);
```

* endpoint: 要连接的对端
* protocol: 指定的协议处理器

### 基础数据报协议模块

#### DatagramProtocol

```c++
class DatagramProtocol;
```

##### 开始时的回调

```c++
virtual void startProtocol();
```

##### 结束时的回调

```c++
virtual void stopProtocol();
```

##### 收到数据报时的回调

```c++
virtual void datagramReceived(Byte *datagram, size_t length, Address address) = 0;
```

* datagram: 数据报内容
* length: 数据报长度
* address: 对端地址

##### 连接拒绝时的回调

```c++
virtual void connectionRefused();
```

##### 连接出错时的回调

```c++
virtual void connectionFailed(std::exception_ptr reason);
```

* reason: 出错原因

##### 获取关联的反应器

```c++
Reactor* reactor();
```

##### 发送数据报

```c++
void write(const Byte *datagram, size_t length, const Address &address={});
void write(const ByteArray &datagram, const Address &address={});
void write(const char *datagram, const Address &address={});
void write(const std::string &datagram, const Address &address={});
```

* datagram: 数据报内容
* length: 数据报长度
* address: 对端地址

##### 连接对端

```c++
void connect(const Address &address);
```

* address: 对端地址

##### 关闭连接

```c++
void loseConnection();
```

##### 获取是否允许广播

```c++
bool getBroadcastAllowed() const;
```

##### 设置是否允许广播

```c++
void setBroadcastAllowed(bool enabled);
```

* enabled: 是否允许广播

##### 获取本地地址

```c++
std::string getLocalAddress() const;
```

##### 获取本地端口

```c++
unsigned short getLocalPort() const;
```

##### 获取对端地址

```c++
std::string getRemoteAddress() const;
```

##### 获取对端端口

```c++
unsigned short getRemotePort() const;
```

### Web模块(待实现)

### WebSocket模块

#### WebSocketProtocol

```c++
class WebSocketProtocol: public Protocol, public std::enable_shared_from_this<WebSocketProtocol>;
```

#### 成功打开时的回调

```c++
virtual void onOpen();
```

#### 收到消息时的回调

```c++
virtual void onMessage(ByteArray payload, bool isBinary);
```

* payload: 消息的内容
* isBinary: 消息是否二进制

#### 收到Ping时的回调

```c++
virtual void onPing(ByteArray payload);
```

* payload: 附带的消息

#### 收到Pong时的回调

```c++
virtual void onPong(ByteArray payload);
```

* payload: 附带的消息

#### 连接关闭时的回调

```c++
virtual void onClose(bool wasClean, boost::optional<unsigned short> code, boost::optional<std::string> reason);
```

* code: 可选的关闭状态码
* reason: 可选的关闭原因

#### 发送消息

```c++
void sendMessage(const Byte *payload, size_t length, bool isBinary=false, size_t fragmentSize=0, bool sync=false, bool doNotCompress=false);
void sendMessage(const ByteArray &payload, bool isBinary=false, size_t fragmentSize=0, bool sync=false, bool doNotCompress=false);
void sendMessage(const char *payload, bool isBinary=false, size_t fragmentSize=0, bool sync=false, bool doNotCompress=false);
void sendMessage(const std::string &payload, bool isBinary=false, size_t fragmentSize=0, bool sync=false, bool doNotCompress=false);
```

* payload: 消息的内容
* length: 消息的长度
* isBinary: 是否二进制

#### 发送ping包

```c++
void sendPing(const Byte *payload, size_t length);
void sendPing(const ByteArray &payload);
void sendPing(const char *payload);
void sendPing(const std::string &payload);
```

* payload: 附带的消息内容
* length: 附带的消息长度

#### 发送pong包

```c++
void sendPong(const Byte *payload, size_t length);
void sendPong(const ByteArray &payload);
void sendPong(const char *payload);
void sendPong(const std::string &payload);
```

* payload: 附带的消息内容
* length: 附带的消息长度

#### 发送close包

```c++
void sendClose(boost::optional<unsigned short> code=boost::none, boost::optional<std::string> reason=boost::none);
```

* code: 可选的关闭状态码
* reason: 可选的关闭原因

#### 获取自身的shared_ptr对象

```c++
template <typename SelfT>
std::shared_ptr<const SelfT> getSelf() const;

template <typename SelfT>
std::shared_ptr<SelfT> getSelf();
```

#### WebSocketServerProtocol

```c++
class WebSocketServerProtocol: public WebSocketProtocol;
```

#### WebSocketServerFactory

```c++
class WebSocketServerFactory: public Factory;
```

##### 构造函数

```c++
WebSocketServerFactory(std::string url="", StringVector protocols={}, std::string version="", WebSocketHeaders headers={}, unsigned short externalPort=0);
```

* url: 服务的url

#### WebSocketClientProtocol

```c++
class WebSocketClientProtocol: public WebSocketProtocol;
```

#### WebSocketClientFactory

```c++
class WebSocketClientFactory: public ClientFactory;
```

##### 构造函数

```c++
explicit WebSocketClientFactory(std::string url = "", std::string origin = "", StringVector protocols = {}, std::string useragent = "", WebSocketHeaders headers = {}, std::string proxy = "");
```

* url: 连接的url

#### 全局函数

##### 启动WebSocketServer

```c++
ListenerPtr listenWS(Reactor *reactor, std::shared_ptr<WebSocketServerFactory> factory, SSLOptionPtr sslOption=nullptr, const std::string &interface={});
```

* reactor: 关联的反应器
* factory: 创建WebSocketServerProtocol的工厂
* sslOption: ssl选项
* interface: 绑定的地址

##### 创建WebSocketClient

```c++
ConnectorPtr connectWS(Reactor *reactor, std::shared_ptr<WebSocketClientFactory> factory, SSLOptionPtr sslOption=nullptr, double timeout=30.0, const Address &bindAddress={});
```

* reactor: 关联的反应器
* factory: 创建WebSocketClientProtocol的工厂
* sslOption: ssl选项
* timeout: 超时值
* bindAddress: 绑定的地址

### 基础工具模块(待实现)

### 多线程支持(待实现)

### 单元测试模块(待实现)

### 电子邮件模块(待实现)
