## This ia a simple webserver

本服务器是按照作者 [@qinguoyi](https://github.com/qinguoyi) 的思想结构来进行编写的

Mywebserver
-------------
Linux 下C++轻量级Web服务器

* 使用 **线程池 + 非阻塞socket + epoll(暂时LT实现) + 事件处理(暂时Proactor实现)** 的并发模型
* 使用 **状态机** 解析HTTP请求报文，支持解析**GET和POST**请求
* 可以请求服务器**图片和视频文件**, 暂时没添加数据库操作
* 实现了 **同步/异步** 可设置的日志系统，记录服务器运行状态
* 经Webbench压力测试可以实现**上万的并发连接**数据交换


目录
-----

| [概述](#概述) | [压力测试](#压力测试) | [快速运行](#快速运行) | [个性化设置](#个性化设置) |[致谢](#致谢) |

概述
----------

> * C/C++/html
> * B/S模型
> * [线程同步机制包装类](https://github.com/jzy916789135/MyServer/tree/master/locker.h)
> * [http连接请求处理类](https://github.com/jzy916789135/MyServer/tree/master/http)
> * [半同步/半反应堆线程池](https://github.com/jzy916789135/MyServer/tree/master/threadpool)
> * [定时器处理非活动连接](https://github.com/jzy916789135/MyServer/tree/master/timer)
> * [同步/异步日志系统](https://github.com/jzy916789135/MyServer/tree/master/log)


压力测试
-------------
使用Webbench对服务器进行压力测试，可实现上万的并发连接(webbench暂时没有写在项目中，需要自行下载)

crisp12345@ubuntu:~/webbench-1.5 $ webbench -c 10500 -t 5 http://192.168.172.128:9006/

Benchmarking: GET http://192.168.172.128:9006/
10500 clients, running 5 sec.

Speed=909636 pages/min, 1697964 bytes/sec.
Requests: 75803 susceed, 0 failed.

> * 并发连接总数：10500
> * 访问服务器时间：5s
> * 所有访问均成功

快速运行
------------
* 服务器测试环境
	* Ubuntu版本16.04
	* MySQL版本5.7.29
* 浏览器测试环境
	* Windows、Linux均可
	* Chrome
	* FireFox
	* 其他浏览器暂无测试
* build

    ```C++
    sh ./build.sh
    ```

* 启动server

    ```C++
    ./server
    ```
* 浏览器端

	* 默认端口：9006


个性化设置

------
目前所有支持的设置均在main（）函数中进行一次设置
* 自定义端口号
```C++
  port  //
```
* 选择日志写入方式
```C++
  logWriteType // 0，同步写入; 1，异步写入
```
* -t，线程数量
```C++
  threadNum // 
```
* 关闭日志
```C++
  logFlag // 0，打开日志; 1，关闭日志
```


致谢
------------

感谢以下朋友: [@qinguoyi](https://github.com/qinguoyi)

