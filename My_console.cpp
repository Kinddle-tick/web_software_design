//
// Created by Kinddle Lee on 2021/9/25.
//
#include "My_console.h"
#include <cstdlib>

//Base类的有一个参数的构造函数
Base_console::Base_console(char* name){
    strcpy(m_Name, name);//拷贝
}
//Base类没有参数的构造函数
Base_console::Base_console() {
    strcpy(m_Name, "Default-name");
}
//Base类析构函数 同时其实也是虚函数
Base_console::~Base_console(){
    sleep(1);
    close(m_server_id);
}

//UDP类Console的两个构造函数函数，继承自Base
UDP_console::UDP_console(char *string) : Base_console(string){}
UDP_console::UDP_console():Base_console(){}
//UDP类Console的析构函数
UDP_console::~UDP_console(){
    sleep(1);
    close(m_server_id);
}

//TCP类Console的两个构造函数,继承自Base
TCP_console::TCP_console(char *string):Base_console(string) {}
TCP_console::TCP_console():Base_console() {}
//TCP类Console的析构函数
TCP_console::~TCP_console(){
    sleep(1);
    if(m_client_id){
        ::shutdown(m_client_id,SHUT_RDWR);
        close(m_client_id);
    }
    close(m_server_id);
}

//SHELL类Console的两个构造函数,继承自Base
SHELL_console::SHELL_console(char* string):Base_console(string) {}
SHELL_console::SHELL_console():Base_console(){}
//SHELL类Console的析构函数
SHELL_console::~SHELL_console(){
    sleep(1);
    close(m_server_id);
};


//UDP类Consaole的初始化函数 主要完成了地址绑定和获得套接字关键字的工作
void UDP_console::init(const char *self_addr, int self_port) {
    sockaddr_in server_addr{};
    if ((m_server_id = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return;
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(self_port);
    server_addr.sin_addr.s_addr = inet_addr(self_addr);

    if(bind(m_server_id, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind socket failed...");
        close(m_server_id);
        exit(2);
    };
}
//UDP类Console的"listen"函数 主要是获取GUI的地址和端口号为后续工作作准备
int UDP_console::listen(int max_connect) {
    std::cout<<"you may have to run GUI by yourself in some Reasonable format"<<std::endl;
    sleep(1);
    //不能确定客户端的地址和端口号，在此处模拟accept操作阻塞等待一次GUI的信息获取client地址等
    ssize_t recvlen;
    char recvbuffer[128]={0};
    printf("ready for connect...\n");

    recvlen = read(recvbuffer, sizeof(recvbuffer), (sockaddr*)&m_client_addr, &m_client_addrLen);
    if ((recvlen)<0){
        perror("Receive message failed..");
        close(m_server_id);
        exit(3);
    }

    std::cout<<"connect set up!\n"<<std::endl;
    printf("GUI addr:%s:%d\n", (char *)inet_ntoa(m_client_addr.sin_addr), htons(m_client_addr.sin_port));
    return 1;
}

//UDP类Console的write 基于sendto构成
ssize_t UDP_console::write(const char *buf, int buf_size) {
    return sendto(m_server_id, buf, buf_size,
                  0, (struct sockaddr*)&m_client_addr, sizeof(m_client_addr));
}
//UDP类Console的read 基于recvfrom构成
ssize_t UDP_console::read(char *buf, int buf_size, sockaddr *from, unsigned int *len) {
    return recvfrom(m_server_id, buf, buf_size, 0, from, len);
}

//TCP类Console的初始化函数 主要完成了地址绑定和获得套接字关键字的工作
void TCP_console::init(const char *self_addr, int self_port) {
    sockaddr_in server_addr{};
    if ((m_server_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return;
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(self_port);
    server_addr.sin_addr.s_addr = inet_addr(self_addr);

    if(bind(m_server_id, (sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind socket failed...");
        close(m_server_id);
        exit(2);
    };
}
//TCP类Console的"listen"函数 主要是建立连接获得描述符
int TCP_console::listen(int max_connect) {
    if(::listen(m_server_id,max_connect)==-1){
        perror("connect fail ");
        return 1;
    }
    std::cout<<"you may have to run GUI by yourself in some Reasonable format"<<std::endl;
    sleep(1);

    printf("ready for connect...\n");
    m_client_id = ::accept(m_server_id,(sockaddr*)&m_client_addr, &m_client_addrLen);
    if (m_client_id<0){
        perror("accept() failed..");
        close(m_server_id);
        exit(3);
    } else{
        std::cout<<"connect set up!\n"<<std::endl;
        printf("GUI addr:%s:%d\n", (char *)inet_ntoa(m_client_addr.sin_addr), htons(m_client_addr.sin_port));
    }
    return 0;
}

//TCP类Console的write 基于write构成
ssize_t TCP_console::write(const char *buf, int buf_size) {
    return ::write(m_client_id,buf,buf_size);
}
//TCP类Console的read 基于read构成
ssize_t TCP_console::read(char *buf, int buf_size, sockaddr *from, unsigned int *len) {
    return ::read(m_client_id,buf,buf_size);
}

void SHELL_console::init(const char *self_addr, int self_port) {}//shell需要做什么吗？ 什么都不需要...写了只是意思一下罢了（
int SHELL_console::listen(int max_connect) {return 0;}//shell需要做什么吗？ 什么都不需要...写了只是意思一下罢了（
//这里的read和write只是做做样子... 请在实际使用的时候创建子类并根据实际情况重写虚函数...qwq
ssize_t SHELL_console::write(const char *buf, int buf_size) {return fwrite(buf,1,buf_size,stdout);}
ssize_t SHELL_console::read(char *buf, int buf_size, sockaddr *from, unsigned int *len) {return fread(buf,1,buf_size,stdin);}


