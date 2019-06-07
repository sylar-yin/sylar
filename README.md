# sylar

## 视频教程地址
0. [C++高级教程-从零开始开发服务器框架(sylar)](https://www.bilibili.com/video/av53602631/ "")
1. [C++服务器框架01_日志系统01](https://www.bilibili.com/video/av52778994/?from=www.sylar.top "")
2. [C++服务器框架02_日志系统02_logger](https://www.bilibili.com/video/av52906685/?from=www.sylar.top "")
3. [C++服务器框架03_日志系统03_appender](https://www.bilibili.com/video/av52906934/?from=www.sylar.top "")
4. [C++服务器框架04_日志系统04_formatter](https://www.bilibili.com/video/av52907828/?from=www.sylar.top "")
5. [C++服务器框架05_日志系统05_formatter2](https://www.bilibili.com/video/av52907987/?from=www.sylar.top "")
6. [C++服务器框架06_日志系统06_编译调试](https://www.bilibili.com/video/av52908593/?from=www.sylar.top "")
7. [C++服务器框架07_日志系统07_完善日志系统](https://www.bilibili.com/video/av52908471/?from=www.sylar.top "")
8. [C++服务器框架08_日志系统08_完善日志系统2](https://www.bilibili.com/video/av52908727/?from=www.sylar.top "")
9. [C++服务器框架09_配置系统01_基础配置](https://www.bilibili.com/video/av52909181/?from=www.sylar.top "")
10. [C++服务器框架10_配置系统02_yaml](https://www.bilibili.com/video/av52909223/?from=www.sylar.top "")
11. [C++服务器框架11_配置系统03_yaml整合](https://www.bilibili.com/video/av52909256/?from=www.sylar.top "")
12. [C++服务器框架12_配置系统04_复杂类型的支持](https://www.bilibili.com/video/av52990220/?from=www.sylar.top "")
13. [C++服务器框架13_配置系统05_更多stl容器支持](https://www.bilibili.com/video/av52991045/?from=www.sylar.top "")
14. [C++服务器框架14_配置系统06_自定义类型的支持](https://www.bilibili.com/video/av52992071/?from=www.sylar.top "")
15. [C++服务器框架15_配置系统07_配置变更事件](https://www.bilibili.com/video/av52992614/?from=www.sylar.top "")
16. [C++服务器框架16_配置系统08_日志系统整合01](https://www.bilibili.com/video/av52993407/?from=www.sylar.top "")
17. [C++服务器框架17_配置系统09_日志系统整合02](https://www.bilibili.com/video/av52994250/?from=www.sylar.top "")
18. [C++服务器框架18_配置系统10_日志系统整合03](https://www.bilibili.com/video/av52995442/?from=www.sylar.top "")

## 开发环境
Centos7
gcc 9.1
cmake
ragel

## 项目路径
bin  -- 二进制
build -- 中间文件路径
cmake -- cmake函数文件夹
CMakeLists.txt -- cmake的定义文件  
lib -- 库的输出路径  
Makefile
sylar -- 源代码路径
tests -- 测试代码

## 日志系统
1）
    Log4J
    
    Logger (定义日志类别)
       |
       |-------Formatter（日志格式）
       |
    Appender（日志输出地方）
    
    
## 配置系统

Config --> Yaml

yamp-cpp: github 搜
mkdir build && cd build && cmake .. && make install

```cpp
YAML::Node node = YAML::LoadFile(filename);
node.IsMap()
for(auto it = node.begin();
    it != node.end(); ++it) {
        it->first, it->second
}

node.IsSequence()
for(size_t i = 0; i < node.size(); ++i) {
    
}

node.IsScalar();
```

配置系统的原则，约定优于配置：

```cpp
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

//容器片特化，目前支持vector
//list, set, map, unordered_set, unordered_map
// map/unordered_set 支持key = std::string
// Config::Lookup(key) , key相同， 类型不同的，不会有报错。这个需要处理一下
```

自定义类型，需要实现sylar::LexicalCast,片特化
实现后，就可以支持Config解析自定义类型，自定义类型可以和常规stl容器一起使用。

配置的事件机制
当一个配置项发生修改的时候，可以反向通知对应的代码，回调

# 日志系统整合配置系统
```yaml
logs：
    - name: root
      level: (debug,info,warn,error,fatal)
      formatter: '%d%T%p%T%t%m%n'
      appender:
        - type: (StdoutLogAppender, FileLogAppender)
          level:(debug,...)
          file: /logs/xxx.log
```
```cpp
    sylar::Logger g_logger = sylar::LoggerMgr::GetInstance()->getLogger(name);
    SYLAR_LOG_INFO(g_logger) << "xxxx log";
```

```cpp
static Logger::ptr g_log = SYLAR_LOG_NAME("system"); //m_root, m_system-> m_root 当logger的appenders为空，使用root写logger
```

```cpp
//定义LogDefine LogAppenderDefine, 偏特化 LexicalCast,
//实现日志配置解析
```


## 协程库封装

## socket函数库

## http协议开发

## 分布协议

## 推荐系统
