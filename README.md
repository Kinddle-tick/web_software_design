# duanjingshan/Unit-5



## Getting Started

Download links:

SSH clone URL: ssh://git@git.jetbrains.space/thepidupark/duanjingshan/Unit-2.git

HTTPS clone URL: https://git.jetbrains.space/thepidupark/duanjingshan/Unit-2.git



These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

## Prerequisites

What things you need to install the software and how to install them.

```
只需要普通的加载一遍CMAKE 然后分别编译S_client.cpp和S_server.cpp 
然后准备python环境并安装python中pyqt相关的软件包就可以了
```

## Deployment
（项目三，简单文件程序）
1. 在New_server和New_client根目录下，创建client_data和server_data文件夹，server_data文件夹中放入user_sheet.csv 按照用户名,密码的格式存放信息，最好再根据用户名在两个数据库中创建与用户名相同的文件夹。
2. 任意顺序启动New_server和New_client
3. 主要的操作在client中完成 尝试键入help获取提示

####Tips:
1. 聊天功能从项目二一脉相承。
2. 服务器与客户端是由C++构建的，采用的select机制完成。中间的链接是TCP链接。
   ```
   server(c++) ┳━━━ client(c++) ┳━━━ GUI(py)
               ┗━━━ client(c++) ┗━━━ GUI(py)
                TCP            TCP/UDP
   ```
3. 可以通过修改my_generic_definition.h中的部分常量来达到自己想要的实验效果，当然要记得先编译。具体每个常量产生的作用在该文件注释中会有提到。
4. 可以通过修改server_source.h和client_source.h中的DEBUG_LEVEL宏定义来观察更多的调试信息
5. 如果出现关于htonll不存在的问题 全局替换成htobe64即可（同理ntohll换成be64toh)
## Resources

Add links to external resources for this project, such as CI server, bug tracker, etc.
