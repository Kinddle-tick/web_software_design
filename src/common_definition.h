//
// Created by Kinddle Lee on 2021/12/26.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_COMMON_DEFINITION_H
#define WEB_SOFTWARE_DESIGN_FIVE_COMMON_DEFINITION_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_addr()
#include <unistd.h>     //close()
#include <iostream>
#include <cstring>
#include <fstream>
#include <random>

#include<sys/stat.h>
#include<fcntl.h>
#include<cerrno>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
//您的数据库的位置
const int kUsernameLength =30;  //最大用户名长度,不大于kBufferSzie
const int kHeaderSize = 20;     //每个包的头部长度，不小于20
const int kBufferSize = 1004;   //每个包数据部分的长度，不小于kUsername
const int kServerPort = 13503;  //服务器的端口号

const int kUserPathMaxLength = (512 - kUsernameLength - sizeof(unsigned int));
//用户发送路径的最大长度，自适应
const int kGenericErrorProb = 0;  //除了ack以外的报文，第一次发送时丢包的概率
const int kTimingErrorProb = 0;     //计时器重传时丢包的概率
const int kGenericAckProb = 0;     //ack报文的丢包概率，这个一般不要超过一百。否则一次完整的会话将难以结束，但不代表不可以试试，因为做过相关的预防了。

//（指ack报文没有超时重传机制，仅由Fin报文触发，fin报文只重传三次）
/*几种组合:
 * kGenericErrorProb = 100，kTimingErrorProb = 0，kGenericAckProb = 20。
 * debug--第一次发送必定出错，用定时器发送必定成功,能够最大限度的调试重传机制是否完备
 *
 * kGenericErrorProb = kTimingErrorProb = kGenericAckProb = N<100
 * 最能模拟现实信道的方案
 *
 * */

const double kGenericTimeInterval = 0.5;

typedef int SocketFileDescriptor;
typedef int FileDescriptor;
typedef char UserNameString[kUsernameLength];

enum State : uint8_t {
    kNull =0,
    kOffline,//既可以标志客户端自身 也可以作为服务端标记客户端的凭据
    kHalfLink,
    kOnline,
    kShutdown, //表示该链接已经结束，将会放到之后清理。

    kChapChallenging,
    kChapResponse,
    kChapSuccess,
    kChapFailure,

    kControlLogin,
    kControlLogout,//10
    kControlRegister,
    kControlChangePassword,
    kControlChangeUsername,
    kControlLs,
    kControlCd,
    kControlUnregistered,
    kControlRegisterSuccess,
    kControlDuplicateRegister,
    kControlDuplicateLogin,
    kControlClientInfo,//20

    kFileUpload,
    kFileDownload,
    kFileRequest,// 本来想与download合并后来想想分开更清楚吧
    kFileResponse,
    kFileInit,
    kFileTransporting,
    kFileAck,
    kFileError,
    kFileEnd,

    kTimerInit,//30
    kTimerWorking,
    kTimerDisable,

};

enum Protocol : uint8_t {
    kProtoControl,
    kProtoMessage,
    kProtoChap,
    kProtoFile,
    kProtoGeneralFinish,
    kProtoGeneralAck,
};

enum SocketErrorCode: uint8_t {
    kSocketErrorConnectRefused,
};
//static 拒绝将此变量发送到连接器 避免出现重复定义
//在头文件中定义常量字符串的时候万万不可忘记 否则duplicate symbol ' ' in:就会找上你家（不是
static const char* kStateDescription[]={
        "\033[0m\033[31mOffline\033[0m",
        "\033[0m\033[35mLoginTry\033[0m",
        "\033[5m\033[32mOnline\033[0m",
};

union header{
    uint8_t chr[kHeaderSize]={0};
    Protocol proto;
    struct {
        Protocol proto;
        State detail_code;
        uint8_t timer_id_tied;
        uint8_t timer_id_confirm;
        uint32_t data_length;
        uint8_t zero[kHeaderSize - 8];
    }base_proto;
    struct {
        uint8_t proto=kProtoChap;
        uint8_t chap_code;
        uint8_t timer_id_tied;
        uint8_t timer_id_confirm;
        uint32_t data_length;
        uint32_t number_count;
        uint32_t sequence;
        uint8_t one_data_size;
    }chap_proto;
    struct {
        uint8_t proto=kProtoMessage;
        uint8_t zero_code;
        uint8_t timer_id_tied;
        uint8_t timer_id_confirm;
        uint32_t data_length;
    }msg_proto;
    struct{
        uint8_t proto = kProtoControl;
        uint8_t ctl_code;
        uint8_t timer_id_tied;
        uint8_t timer_id_confirm;
        uint32_t data_length;
    }ctl_proto;
    struct{
        uint8_t proto = kProtoFile;
        uint8_t file_code;
        uint8_t timer_id_tied;
        uint8_t timer_id_confirm;
        uint32_t data_length;
        uint32_t frame_transport_length;
        uint32_t session_id;
        uint32_t sequence;
    }file_proto;
};

union data{
    struct {
        UserNameString userName;
        uint32_t answer;
    }chap_response;
    struct{
        UserNameString userName;
        char msg_data[kBufferSize - sizeof(UserNameString)];
    }msg_general;
    struct {
        UserNameString userName;
    }ctl_login;
    struct {
        char chr[kBufferSize];
    }ctl_ls;
    struct {
        uint32_t client_number;
        uint8_t client_append;//是否是增添模式，0表示是第一个报文（需要清空列表重新记录），其他数字则表示是新增的报文
        char chr[kBufferSize-sizeof(uint32_t)-sizeof(uint8_t)];
    }ctl_client_info;
    struct{
        timespec init_time; // 但一般这里不会有初始化 只是看着后面三个规整一点可能比较好（
        char file_path[kBufferSize - sizeof(timespec)];
//        char file_path_server[(kBufferSize - sizeof(timespec)/2)];
//        char file_path_client[(kBufferSize - sizeof(timespec)/2)];
    }file_upload;
    struct{
        timespec init_time; // 但一般这里不会有初始化 只是看着后面三个规整一点可能比较好（
        char file_path[kBufferSize - sizeof(timespec)];
//        char file_path_server[(kBufferSize - sizeof(timespec)/2)];
//        char file_path_client[(kBufferSize - sizeof(timespec)/2)];
    }file_download;
    struct {
        timespec init_time;
        char file_path[kBufferSize - sizeof(timespec)];
    }file_request;
    struct {
        timespec init_time;
        char file_path[kBufferSize - sizeof(timespec)];
    }file_response;
    struct {
        uint32_t CRC_32;
        char file_data[kBufferSize - sizeof(uint32_t)];//未来可能加入CRC
    }file_transport;
    struct {
        timespec end_time;
    }file_end;
};
#endif //WEB_SOFTWARE_DESIGN_FIVE_COMMON_DEFINITION_H
