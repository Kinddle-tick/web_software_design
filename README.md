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
（项目二，双人聊天程序）
1. 启动S_server 当其出现`wait for client connect!`的字样时进行步骤2
2. 运行client_GUI.py脚本**两次**，可以选择传入参数或者无参数启动（无参数启动时会有关于参数的使用例）
3. 分别单击两个聊天窗口的"客户端信息"按钮，确定当前聊天窗口准备连接的客户端地址，并确定该地址可用 *_(主要是要保证两个客户端不会使用相同的地址)_*
4. 分别单击两个窗口的"链接客户端按钮"，绑定聊天窗口与客户端。而客户端在启动时会自动链接步骤1中的服务器，可以观察服务器的反应来判断链接是否完成。
5. 接下来就可以使用发送按钮进行聊天了

####Tips:
1. 服务器是按照类似洪泛的规则转发从客户端发来的消息的，但当只有一个客户端链接时，将是个回音壁服务器。
2. 服务器与客户端是由C++构建的，采用的select机制完成。中间的链接是TCP链接。
   ```
   server(c++) ┳━━━ client(c++) ┳━━━ GUI(py)
               ┗━━━ client(c++) ┗━━━ GUI(py)
                TCP            TCP/UDP
   ```
3. 因为是双人聊天 所以昵称部分没有深入去研究如何完成，在项目五中会实现

## Resources

Add links to external resources for this project, such as CI server, bug tracker, etc.
