//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#include "my_generic_definition.h"
#include <list>
//region 宏定义
//#define backlog 16
//#define kDirLength 30 //请注意根据文件夹的名称更改此数字

const int backlog = 16;
const int  kDirLength = 30;//请注意根据文件夹的名称更改此数字
//endregion

//region 结构体声明
struct client_session{
    SocketFileDescriptor socket_fd = 0;
    UserNameString nickname = "";
    char now_path[kUserPathMaxLength]={0};
    State state = kOffline;
    clock_t tick = 0;
};
struct user_info{
    UserNameString user_name;
    unsigned int password;
    char root_path[kUserPathMaxLength];
};
struct chap_session{
    uint32_t sequence;
    uint32_t answer;
    client_session * client;
    clock_t tick;
};
struct FileSession{
    uint32_t session_id;
    uint32_t sequence;
    FileDescriptor file_fd;
    FILE * file_ptr;
    State state;
    clock_t tick;
};
//endregion

//region 函数原型声明
int EventReceiveStdin();
int EventNewClient();
int EventClientMessage(client_session *client);

unsigned int ActionControlUnregistered(client_session *);
unsigned int ActionChapChallenge(client_session *client);
unsigned int ActionChapJustice(const char *receive_packet_total, client_session *client);
int ActionControlLogin(const char*, client_session*);
int ActionControlLsResponse(client_session*);
int ActionFileResponse(const char*, client_session*);
int ActionFileTranslating(FileSession*, client_session*);
int ActionFileAckReceived(const char*, client_session*);
int ActionFileEndSend(const char*, client_session*);
int ActionMessageProcessing(const char *receive_packet_total, client_session *client);
//endregion

extern int conn_server_fd;
extern unsigned int session_id;
extern std::list<client_session>* client_list;
extern std::list<chap_session>* chap_list;
extern std::list<user_info>* user_list;
extern std::list<FileSession>* file_list;

#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
