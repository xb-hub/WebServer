# WebServer
- 运行环境: ubuntu18.04
- 编译器: CLion
## 编译运行
- 终端编译运行
```
编译：
mkdir build
cd build
cmake ..
make
运行：
1. 使用线程池:
./server [端口号] [服务器文件目录路径] pool [最大线程数]
2. 不使用线程池:
./server [端口号] [服务器文件目录路径] nopool
```
- CLion编译器
```
在运行配置中配置运行参数
```
- 客户端
```
打开浏览器，输入url
127.0.0.1:[端口号]              // 返回index.html
127.0.0.1:[端口号]/demo.png     // 返回demo.png