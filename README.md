# 视频地址
[\[C++高级教程\]从零开始开发服务器框架(sylar)](https://www.bilibili.com/video/av53602631/?from=www.sylar.top "")
# 视频教程内容：
## 1.日志模块
支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能
流式日志使用：SYLAR_LOG_INFO(g_logger) << "this is a log";
格式化日志使用：SYLAR_LOG_FMT_INFO(g_logger, "%s", "this is a log");
支持时间,线程id,线程名称,日志级别,日志名称,文件名,行号等内容的自由配置
## 2.配置模块
采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。支持级别格式的数据类型，支持STL容器(vector,list,set,map等等),支持自定义类型的支持（需要实现序列化和反序列化方法)使用方式如下：
```cpp
static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
	sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
```
定义了一个tcp连接超时参数，可以直接使用 g_tcp_connect_timeout->getValue() 获取参数的值，当配置修改重新加载，该值自动更新
上述配置格式如下：
```sh
tcp:
    connect:
            timeout: 10000
```
## 3.线程模块
线程模块，封装了pthread里面的一些常用功能，Thread,Semaphore,Mutex,RWMutex,Spinlock等对象，可以方便开发中对线程日常使用
为什么不适用c++11里面的thread
本框架是使用C++11开发，不使用thread，是因为thread其实也是基于pthread实现的。并且C++11里面没有提供读写互斥量，RWMutex，Spinlock等，在高并发场景，这些对象是经常需要用到的。所以选择了自己封装pthread
## 4.协程模块
协程：用户态的线程，相当于线程中的线程，更轻量级。后续配置socket hook，可以把复杂的异步调用，封装成同步操作。降低业务逻辑的编写复杂度。
目前该协程是基于ucontext_t来实现的，后续将支持采用boost.context里面的fcontext_t的方式实现
## 5.协程调度模块
协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，也可以指定协程在固定的线程中执行。是一个N-M的协程调度模型，N个线程，M个协程。重复利用每一个线程。
## 6.IO协程调度模块
继承与协程调度器，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）,支持Socket读写时间的添加，删除，取消功能。支持一次性定时器，循环定时器，条件定时器等功能
## 7.Hook模块
hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。通过hook模块，可以使一些不具异步功能的API，展现出异步的性能。如（mysql）
## 8.Socket模块
封装了Socket类，提供所有socket API功能，统一封装了地址类，将IPv4，IPv6，Unix地址统一起来。并且提供域名，IP解析功能。
## 9.ByteArray序列化模块
ByteArray二进制序列化模块，提供对二进制数据的常用操作。读写入基础类型int8_t,int16_t,int32_t,int64_t等，支持Varint,std::string的读写支持,支持字节序转化,支持序列化到文件，以及从文件反序列化等功能
## 10.TcpServer模块
基于Socket类，封装了一个通用的TcpServer的服务器类，提供简单的API，使用便捷，可以快速绑定一个或多个地址，启动服务，监听端口，accept连接，处理socket连接等功能。具体业务功能更的服务器实现，只需要继承该类就可以快速实现
## 11.Stream模块
封装流式的统一接口。将文件，socket封装成统一的接口。使用的时候，采用统一的风格操作。基于统一的风格，可以提供更灵活的扩展。目前实现了SocketStream
## 12.HTTP模块
采用Ragel（有限状态机，性能媲美汇编），实现了HTTP/1.1的简单协议实现和uri的解析。基于SocketStream实现了HttpConnection(HTTP的客户端)和HttpSession(HTTP服务器端的链接）。基于TcpServer实现了HttpServer。提供了完整的HTTP的客户端API请求功能，HTTP基础API服务器功能
## 13.Servlet模块
仿照java的servlet，实现了一套Servlet接口，实现了ServletDispatch，FunctionServlet。NotFoundServlet。支持uri的精准匹配，模糊匹配等功能。和HTTP模块，一起提供HTTP服务器功能
## 14.其他相关
联系方式：
 QQ：564628276
 邮箱：564628276@qq.com
 微信：sylar-yin
 QQ群：8151915（sylar技术群）
个人主页：www.sylar.top
github:https://github.com/sylar-yin/sylar
