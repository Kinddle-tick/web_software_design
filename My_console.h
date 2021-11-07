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

typedef int SOCKET_ID;

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
