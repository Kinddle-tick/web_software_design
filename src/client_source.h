//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#include "my_generic_definition.h"
#include <list>

//region 宏定义与枚举类型
#define DEBUG_LEVEL 0
#define DIR_LENGTH 30 //请注意根据文件夹的名称更改此数字
enum EventReturnState :int{
    kOtherError = -4,
    kStdinExit,
    kClientExit,
    kServerShutdown,
    kNormal,
    kConnectServer,
};
//endregion

//region 结构体声明
struct file_session{
    uint32_t session_id;
    uint32_t sequence;
    int file_fd;
    clock_t init_clock;
    uint32_t init_sequence;
};

struct fd_info{
    int fd;
    USER_NAME nickname;
};

struct client_self_data{
    USER_NAME confirmUserName = "";
    USER_NAME tmpUserName = "";
    uint32_t password = 0;
    State state = Offline;
    const char client_data_root[DIR_LENGTH] = "client_data/";
    char client_data_dir_now[DIR_LENGTH] = "client_data/";
};

struct help_doc{
    const char * cmd_name = nullptr;
    const char * cmd_description = nullptr;
    const char * cmd_pattern = nullptr;
};
//endregion

//region 函数原型声明
ssize_t EventServerMsg();
int EventNewGui();
ssize_t EventGUIMsg(fd_info*);
int EventStdinMsg();
void BaseActionInterrupt();
int ActionSendMessageToServer(const char*);
int ActionMessageFromServer(const char*);
int ActionCHAPResponse(const char*);
int ActionPrintConsoleHelp(const char*);
int ActionConnectServer();
int ActionControlLogin();
int ActionControlLs();
int ActionControlMonitor();
int ActionFileRequest(const char*);
int ActionFileResponseReceived(const char * );
int ActionFileTransportingReceived(const char*);
int ActionFileACKSend(uint32_t, uint32_t);
int ActionFileErrorSend(uint32_t, uint32_t);
int ActionFileEndReceived(const char*);
//endregion

extern int conn_client_fd,gui_server_fd;
extern bool cue_flag;

extern fd_set * client_fd_set;
extern std::list<fd_info>* gui_client_list;
extern std::list<file_session>* file_list;
extern int max_fd;
extern struct client_self_data * self_data;

#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
