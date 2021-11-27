//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#include <list>
//region 宏定义
#define backlog 16
#define DIR_LENGTH 30 //请注意根据文件夹的名称更改此数字
#include "my_generic_definition.h"
//endregion

//region 结构体声明
struct client_session{
    int socket_fd = 0;
    USER_NAME nickname = "";
    char now_path[USER_PATH_MAX_LENGTH]={0};
    State state = Offline;
    clock_t tick = 0;
};
struct user_info{
    USER_NAME user_name;
    unsigned int password;
    char root_path[USER_PATH_MAX_LENGTH];
};
struct chap_session{
    uint32_t sequence;
    uint32_t answer;
    client_session * client;
    clock_t tick;
};
struct file_session{
    uint32_t session_id;
    uint32_t sequence;
    int file_fd;
    FILE * file_ptr;
    State state;
    clock_t tick;
};
//endregion

//region 函数原型声明
int EventRcvStdin();
int EventNewClient();
int EventClientMsg(client_session*);

unsigned int ActionControlUnregistered(client_session *);
unsigned int ActionCHAPChallenge(client_session*);
unsigned int ActionCHAPJustice(const char*, client_session*);
int ActionControlLogin(const char*, client_session*);
int ActionControlLsResponse(client_session*);
int ActionFileResponse(const char*, client_session*);
int ActionFileTranslating(file_session*, client_session*);
int ActionFileAckReceived(const char*, client_session*);
int ActionFileEndSend(const char*, client_session*);
int ActionMsgProcessing(const char*, client_session*);
//endregion

extern int conn_server_fd;
extern unsigned int session_id;
extern std::list<client_session>* client_list;
extern std::list<chap_session>* chap_list;
extern std::list<user_info>* user_list;
extern std::list<file_session>* file_list;

#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
