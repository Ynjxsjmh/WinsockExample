<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-generate-toc again -->
**Table of Contents**

- [更改说明](#更改说明)
- [运行说明](#运行说明)
- [感想](#感想)

<!-- markdown-toc end -->


# 更改说明
代码来自 [Single Server With Multiple Clients: A Simple C++ Implementation](https://www.codeproject.com/Articles/7785/Single-Server-With-Multiple-Clients-A-Simple-Cplus)

原文提到的 ariticle 1 和 ariticle 2 分别指 [A light-weighted client/server socket class in C++](https://www.codeproject.com/Articles/7108/A-light-weighted-client-server-socket-class-in-C)  [Producer/Consumer Implementation Using Thread, Semaphore and Event](https://www.codeproject.com/Articles/7653/Producer-Consumer-Implementation-Using-Thread-Sema)

原代码在 `singleservermultipleclient_src.zip` 里，但是我试了不可直接运行。现在的是可以直接运行的，做的主要更改有：

1. 参考 [C++ - Invalid initialization of non-const reference of type](https://stackoverflow.com/questions/29210710/c-invalid-initialization-of-non-const-reference-of-type)

  ```
  error: invalid initialization of non-const reference of type 'std::__cxx11::string& {aka std::__cxx11::basic_string<char>&}' from an rvalue of type 'std::__cxx11::string {aka std::__cxx11::basic_string<char>}'|
  ```
  
  主要就是构造一个临时变量 `std::string temp`，将 temp 作为参数传进去。
  
2. 参考 [error: invalid conversion from 'int' to 'std::_Ios_Openmode'](https://stackoverflow.com/a/24746390/10315163)

   主要就是把涉及到的 `int` 全部换成 `std::ios_base::openmode`

3. 把类似 `#include "..\myEvent\myEvent.h"` 全部换成 `#include "myEvent.h"`

# 运行说明
我是把它作为一个 CodeBlocks 项目运行的，具体的就是分别给 client 和 server 创建一个 project，然后把对应的文件一股脑们 add 进去。CodeBlocks 默认会创建一个 main.cpp，把这个文件删了，因为已经写了 main。

运行之前还得更改 client 下的 `serverConfig.txt` 文件，将里面的内容替换成服务器的 IP（启动服务器即可）。

若想运行多个 client，在 CodeBlocks 下是不可以的。需要把 bin/Debug 下的 exe 拷到和 `serverConfig.txt` 同级的目录下。

# 感想
本以为这个是 Client-Server-Client 的客户端通过服务端转发给客户端发消息的，捣鼓老半天没想到不是，白费劲了。

