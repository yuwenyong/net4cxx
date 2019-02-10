
# net4cxx

## Requirements

### Windows（以msys2-mingw64为例）

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

### linux（以ubuntu为例）

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
* 源码编译安装cmake,openssl,boost或运行以下命令

```bash
brew update
brew install openssl cmake boost
```

## Build

* 依次运行以下命令即可编译本模块

```bash
cd net4cxx
mkdir build
cd build
cmake ..
make
```

## Remark

* 我很懒，文档聊胜于无

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

class TCPServerApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        TCPServerEndpoint endpoint(reactor(), "28001");
        endpoint.listen(std::make_shared<EchoFactory>());
    }
};

int main(int argc, char **argv) {
    TCPServerApp app;
    app.run(argc, argv);
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
class TCPServerApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        serverFromString(reactor(), "tcp:28001")->listen(std::make_shared<EchoFactory>());
        serverFromString(reactor(), "ssl:28002:privateKey=test.key:certKey=test.crt")->listen(std::make_shared<EchoFactory>());
        serverFromString(reactor(), "unix:/var/foo/bar")->listen(std::make_shared<EchoFactory>());
    }
};

int main(int argc, char **argv) {
    TCPServerApp app;
    app.run(argc, argv);
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

class TCPClientApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        reactor()->connectTCP("localhost", "28001", std::make_shared<WelcomeFactory>());
    }
};

int main(int argc, char **argv) {
    TCPClientApp app;
    app.run(argc, argv);
    return 0;
}

```

* 以上实现了一个最简单的例子,客户端向服务器打了个招乎,随后关闭连接;
* 将connectTCP替换成connectSSL或者connectUNIX能分别建立ssl或者unix客户端连接;
* 与服务器一样支持从字符串构建客户端连接,调用clientFromString即可,不再赘述;


```c++
class TCPClientApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        reactor()->connectTCP("localhost", "28001", std::make_shared<OneShotFactory>(std::make_shared<WelcomeMessage>()));
    }
};
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

class UDPServerApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        reactor()->listenUDP(28002, std::make_shared<Echo>());
    }
};


int main(int argc, char **argv) {
    UDPServerApp app;
    app.run(argc, argv);
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

class UDPClientApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        reactor()->connectUDP("127.0.0.1", 28002, std::make_shared<EchoClient>());
    }
};


int main(int argc, char **argv) {
    UDPClientApp app;
    app.run(argc, argv);
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

    void tick(Reactor *reactor) {
        _tickCount += 1;
        broadcast(StrUtil::format("tick %d from server", _tickCount));
        reactor->callLater(1.0, [reactor, this, self=shared_from_this()]() {
           tick(reactor);
        });
    }

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
    int _tickCount{0};
    std::set<WebSocketServerProtocolPtr> _clients;
};


class BroadcastServerProtocol: public WebSocketServerProtocol {
public:
    void onOpen() override {
        getFactory<BroadcastServerFactory>()->registerClient(getSelf<BroadcastServerProtocol>());
    }

    void onMessage(ByteArray payload, bool isBinary) override {
        if (!isBinary) {
            auto msg = StrUtil::format("%s from %s", TypeCast<std::string>(payload), getPeerName());
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


class WebSocketServerApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        auto factory = std::make_shared<BroadcastServerFactory>("ws://127.0.0.1:9000");
        listenWS(reactor(), factory);
        factory->tick(reactor());
    }
};

int main(int argc, char **argv) {
    WebSocketServerApp app;
    app.run(argc, argv);
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
            NET4CXX_LOG_INFO(gAppLog, "Text message received: %s", TypeCast<std::string>(payload));
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


class WebSocketClientApp: public Bootstrapper {
public:
    using Bootstrapper::Bootstrapper;

    void onRun() override {
        auto factory = std::make_shared<BroadcastClientFactory>("ws://127.0.0.1:9000");
        connectWS(reactor(), factory);
    }
};

int main(int argc, char **argv) {
    WebSocketClientApp app;
    app.run(argc, argv);
    return 0;
}
```

* 以上实现了一个会自动定时发消息的websocket客户端

### 开发基于http协议的服务器

 ```c++
 #include "net4cxx/net4cxx.h"
 
 using namespace net4cxx;
 
 std::map<int, std::string> gBookNames;
 
 class Books: public RequestHandler {
 public:
     using RequestHandler::RequestHandler;
 
     DeferredPtr onGet(const StringVector &args) override {
         JsonValue response;
         response["books"] = JsonType::arrayValue;
         for (auto book: gBookNames) {
             JsonValue b;
             b["id"] = book.first;
             b["name"] = book.second;
             response["books"].append(b);
         }
         write(response);
         return nullptr;
     }
 
     DeferredPtr onPost(const StringVector &args) override {
         auto body = boost::lexical_cast<JsonValue>(getRequest()->getBody());
         gBookNames[body["id"].asInt()] = body["name"].asString();
         write(body);
         return nullptr;
     }
 };
 
 
 class Book: public RequestHandler {
 public:
     using RequestHandler::RequestHandler;
 
 
     DeferredPtr onGet(const StringVector &args) override {
         auto bookId = std::stoi(args[0]);
         auto iter = gBookNames.find(bookId);
         if (iter != gBookNames.end()) {
             JsonValue response;
             response["book"]["id"] = iter->first;
             response["book"]["name"] = iter->second;
             write(response);
         } else {
             sendError(404);
         }
         return nullptr;
     }
 
     DeferredPtr onDelete(const StringVector &args) override {
         auto bookId = std::stoi(args[0]);
         auto iter = gBookNames.find(bookId);
         if (iter != gBookNames.end()) {
             JsonValue response;
             response["book"]["id"] = iter->first;
             response["book"]["name"] = iter->second;
             gBookNames.erase(iter);
             write(response);
         } else {
             sendError(404);
         }
         return nullptr;
     }
 
     DeferredPtr onPut(const StringVector &args) override {
         auto bookId = std::stoi(args[0]);
         auto iter = gBookNames.find(bookId);
         if (iter != gBookNames.end()) {
             JsonValue response;
             iter->second = getArgument("name");
             response["book"]["id"] = iter->first;
             response["book"]["name"] = iter->second;
             write(response);
         } else {
             sendError(404);
         }
         return nullptr;
     }
 };
 
 class HTTPServerApp: public Bootstrapper {
 public:
     using Bootstrapper::Bootstrapper;
     
     void onRun() override {
         auto webApp = makeWebApp<WebApp>({
                                                  url<Books>(R"(/books/)"),
                                                  url<Book>(R"(/books/(\d+)/)")
                                          });
         reactor()->listenTCP("8080", std::move(webApp));
     }
 };
 
 
 int main(int argc, char **argv) {
     HTTPServerApp app;
     app.run(argc, argv);
     return 0;
 }
 ```
 
 * 以上实现了一组增删改查的书籍信息的restful接口,使用正则表达式捕获路径参数
 
 ### 开发基于http协议的客户端
 
 ```c++
 #include "net4cxx/net4cxx.h"
 
 using namespace net4cxx;
 
 
 class HTTPClientApp: public Bootstrapper {
 public:
     using Bootstrapper::Bootstrapper;
     
     void onRun() {
         auto request = HTTPRequest::create("https://www.baidu.com/")
                 ->setValidateCert(false)
                 ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
         HTTPClient::create()->fetch(request, [](const HTTPResponse &response){
             std::cout << response.getCode() << std::endl;
             if (response.getError()) {
                 try {
                     response.rethrow();
                 } catch (std::exception &e) {
                     std::cerr << e.what() << std::endl;
                 }
             } else {
                 std::cout << response.getBody() << std::endl;
             }
         })->addCallback([](DeferredValue value) {
             std::cout << "Success" << std::endl;
             return value;
         });
     }
 };
 
 int main(int argc, char **argv) {
     HTTPClientApp app;
     app.run(argc, argv);
     return 0;
 }
 ```
 
 * 以上实现了模拟浏览器抓取百度主页
 
 ### 该网络框架支持多线程，而且是per thread per loop的高效方式
 
 ```c++
 #include "net4cxx/net4cxx.h"
 
 using namespace net4cxx;
 
 
 class Books: public RequestHandler {
 public:
     using RequestHandler::RequestHandler;
 
     DeferredPtr onGet(const StringVector &args) override {
         std::cerr << "ThreadId:" << std::this_thread::get_id() << std::endl;
         JsonValue response;
         response["books"] = JsonType::arrayValue;
         write(response);
         return nullptr;
     }
 };
 
 
 class HTTPServerMTTest: public Bootstrapper {
 public:
     using Bootstrapper::Bootstrapper;
 
     void onRun() override {
         auto webApp = makeWebApp<WebApp>({
                                                  url<Books>(R"(/books/)")
                                          });
         reactor()->listenTCP("8080", std::move(webApp));
     }
 };
 
 
 int main(int argc, char **argv) {
     HTTPServerMTTest app(4, false);
     app.run(argc, argv);
     return 0;
 }
 ```
 
  * 以上实现了4个线程的http服务器，每次处理请求打印出目前连接处于哪个线程
  
  ### http服务的请求处理支持异步操作
  
  ```c++
  #include "net4cxx/net4cxx.h"
  
  using namespace net4cxx;
  
  
  class Books: public RequestHandler {
  public:
      using RequestHandler::RequestHandler;
  
      DeferredPtr prepare() override {
          return testAsyncFunc();
      }
  
      DeferredPtr onGet(const StringVector &args) override {
          return testAsyncFunc2();
      }
  
      DeferredPtr testAsyncFunc() {
          auto request = HTTPRequest::create("https://www.baidu.com/")
                  ->setValidateCert(false)
                  ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
          return HTTPClient::create()->fetch(request, [this, self=shared_from_this()](const HTTPResponse &response){
              std::cout << response.getCode() << std::endl;
              getArgument("name");
          })->addCallbacks([](DeferredValue value) {
              std::cout << "Success" << std::endl;
              return value;
          }, [](DeferredValue value) {
              std::cout << "Fail" << std::endl;
              return value;
          });
      }
  
      DeferredPtr testAsyncFunc2() {
          auto request = HTTPRequest::create("https://www.baidu.com/")
                  ->setValidateCert(false)
                  ->setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:61.0) Gecko/20100101 Firefox/61.0");
          return HTTPClient::create()->fetch(request, [self=shared_from_this()](const HTTPResponse &resp){
              std::cout << resp.getCode() << std::endl;
              JsonValue response;
              response["books"] = JsonType::arrayValue;
              write(response);
          })->addCallbacks([](DeferredValue value) {
              std::cout << "Success" << std::endl;
              return value;
          }, [](DeferredValue value) {
              std::cout << "Fail" << std::endl;
              return value;
          });
      }
  };
  
  
  class HTTPServerAsyncTest: public Bootstrapper {
  public:
      using Bootstrapper::Bootstrapper;
  
      void onRun() override {
          auto webApp = makeWebApp<WebApp>({
                                                   url<Books>(R"(/books/)"),
                                           });
          reactor()->listenTCP("8080", std::move(webApp));
      }
  };
  
  
  int main(int argc, char **argv) {
      HTTPServerAsyncTest app;
      app.run(argc, argv);
      return 0;
  }
  ```
  
  * 这里请求开始处理开始前异步访问了baidu,实际处理前再次访问了baidu,前面的异步操作完成才会开始后面的异步操作，理论上所有的异步操作都能用Deferred对象封装


