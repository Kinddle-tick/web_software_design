//
// Created by Kinddle Lee on 2021/9/25.
//

#ifndef PRACTICE1_1_MY_CONSOLE_H
#define PRACTICE1_1_MY_CONSOLE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_addr()
#include <unistd.h>     //close()
#include <iostream>
#include <string>
#include <fstream>
#include <random>

#include<sys/stat.h>
#include<fcntl.h>
#include<cerrno>

#define USERNAME_LENGTH 30
typedef int SOCKET_ID;
typedef char USER_NAME[USERNAME_LENGTH];

#define HEADER_SIZE 20
#define BUFFER_SIZE 1004
#define SER_PORT 11285

enum State : char {
    Offline = 0,//既可以标志客户端自身 也可以作为服务端标记客户端的凭据
    LoginTry,
    Online,

    CHAPChallenging,
    CHAPResponse,
    CHAPSuccess,
    CHAPFailure,

    CTLLogin,
    CTLRegister,
    CTLChangePassword,
    CTLChangeUsername,
    CTLUnregistered,
};
enum Protocol : char{
    ProtoCTL,
    ProtoMsg,
    ProtoCHAP,
    ProtoFile,
};
const char* State_description[]={
        "\033[0m\033[31mOffline\033[0m",
        "\033[0m\033[35mLoginTry\033[0m",
        "\033[5m\033[32mOnline\033[0m",
};

union header{
    uint8_t chr[HEADER_SIZE]={0};
    uint8_t proto;
    struct {
        uint8_t proto;
        uint8_t zero[HEADER_SIZE - 1];
    }base_proto;
    struct {
        uint8_t proto;
        uint8_t chap_code;
        uint8_t one_data_size;
        uint8_t zero;
        uint32_t data_length;
        uint32_t sequence;
    }chap_proto;
    struct {
        uint8_t proto;
    }msg_proto;
    struct{
        uint8_t proto = ProtoCTL;
        uint8_t ctl_code;
    }ctl_proto;
};

union data{
    struct {
        USER_NAME userName;
        uint32_t answer;
        char other;
    }chap_response;
    struct {
        USER_NAME userName;
    }ctl_login;
};

class Base_console {
protected:
    char m_Name[20]{};

public:
    SOCKET_ID m_server_id{};
    sockaddr_in m_client_addr{};
    unsigned int m_client_addrLen=sizeof(m_client_addr);

    explicit Base_console(char* name);
    Base_console();
    virtual ~Base_console();

    virtual void init(const char *self_addr, int self_port) = 0;
    virtual int listen(int max_connect) = 0;

    virtual ssize_t write(const char* buf,int buf_size) = 0;
    virtual ssize_t read(char *buf, int buf_size, sockaddr *from, unsigned int *len) = 0;
};

class UDP_console :public Base_console{
public:
    explicit UDP_console(char *string);
    UDP_console();
    ~UDP_console() override;

    void init(const char *self_addr, int self_port) override;
    int listen(int max_connect) override;

    ssize_t write(const char* buf,int buf_size) override;
    ssize_t read(char *buf, int buf_size, sockaddr *from, unsigned int *len) override;
};

class TCP_console :public Base_console{
private:
    SOCKET_ID m_client_id{};
public:
    explicit TCP_console(char *string);
    TCP_console();
    ~TCP_console() override;

    void init(const char *self_addr, int self_port) override;
    int listen(int max_connect) override;

    ssize_t write(const char* buf,int buf_size) override;
    ssize_t read(char *buf, int buf_size, sockaddr *from, unsigned int *len) override;
};

class SHELL_console :public Base_console{
public:
    explicit SHELL_console(char* string);
    SHELL_console();
    ~SHELL_console()override;

    void init(const char *self_addr, int self_port) override;
    int listen(int max_connect) override;

    ssize_t write(const char* buf,int buf_size) override;
    ssize_t read(char *buf, int buf_size, sockaddr *from, unsigned int *len) override;

};

#endif //PRACTICE1_1_MY_CONSOLE_H
