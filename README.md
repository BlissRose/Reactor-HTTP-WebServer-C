## 基于 Linux 的高并发 Reactor HTTP 服务器（C 语言版本）

该项目实现了一个简易的多线程 HTTP 服务器，采用 Reactor 模式进行事件驱动。通过阅读与实践
可以学习 Linux 网络编程、事件循环与线程池模型等相关知识。

### 编译依赖

* **GCC**：支持 C99 即可。
* **pthread**：使用多线程需要链接 `-lpthread`。
* 推荐在 Linux 环境下构建与运行。

### 构建步骤

1. 克隆或下载本仓库源码。
2. 在项目根目录执行以下编译命令：

   ```bash
   gcc main.c Buffer.c Channel.c ChannelMap.c EpollDispatcher.c EventLoop.c \
       HttpRequest.c Httpresponse.c TcpConnection.c TcpServer.c \
       ThreadPool.c WorkerThread.c SelectDispatcher.c PollDispatcher.c \
       -lpthread -o server
   ```

   编译完成后会生成可执行文件 `server`（也可以修改为其他名称）。

### 运行方式

1. 根据需要修改 `main.c` 中的静态文件目录（默认写死为示例路径 `chdir("/home/kobe/linux/dabing/luffy")`）。
2. 启动服务器：

   ```bash
   ./server
   ```

   服务器默认监听 `10000` 端口，可在浏览器中访问 `http://<服务器 IP>:10000` 查看效果。

3. 如需自定义端口或工作路径，可参考 `main.c` 中被注释的示例，传入端口和路径参数后重新编译运行。

### 目录结构简介

* `EventLoop*`、`*Dispatcher.c`：事件循环及多种 IO 多路复用实现。
* `TcpServer*`、`TcpConnection*`：TCP 服务器与连接管理。
* `ThreadPool*`、`WorkerThread*`：线程池相关代码。
* `HttpRequest*`、`HttpResponse*`：HTTP 请求解析与响应封装。
* `Buffer*`：简易缓冲区工具。

希望这些说明能帮助新成员快速构建并运行项目，进一步了解各模块的实现。
