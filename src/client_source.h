//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#include "my_generic_definition.h"
#include <list>

//region 宏定义与枚举类型
#define CLIENT_DEBUG_LEVEL 0
//#define kDirLength 30 //请注意根据文件夹的名称更改此数字
//#define CLIENT_DEBUG_LEVEL 0
const int kDirLength = 30; //请注意根据文件夹的名称更改此数字
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
struct FileSession{
    uint32_t session_id;
    uint32_t sequence;
    FileDescriptor file_fd;
    clock_t init_clock;
    uint32_t init_sequence;
};

struct FdInfo{
    SocketFileDescriptor fd;
    UserNameString nickname;
};

struct ClientSelfData{
    UserNameString confirmUserName = "";
    UserNameString tmpUserName = "";
    uint32_t password = 0;
    State state = kOffline;
    const char client_data_root[kDirLength] = "client_data/";
    char client_data_dir_now[kDirLength] = "client_data/";
};

struct HelpDoc{
    const char * cmd_name = nullptr;
    const char * cmd_description = nullptr;
    const char * cmd_pattern = nullptr;
};

class TimerSession{
private:
    int retry_count_;
    clock_t init_tick_;
    int timing_second_;
protected:
    State timer_state_;
public:
    explicit TimerSession(int);
    bool TimerUpdate();
    bool TimeoutJustice() const;
    void set_timing_second(int);
    int get_timing_second() const;
    virtual bool TimerTrigger();
};

class TimerRetranslationSession:public TimerSession{
private:
    char packet_cache_[kHeaderSize+kBufferSize]{0};
    SocketFileDescriptor client_fd_;
    size_t packet_length_;
public:
    TimerRetranslationSession(int,SocketFileDescriptor,const char*,size_t);
    bool TimerTrigger() override;
};

class TimerRemoveSession:public TimerSession{
private:
    bool (* trigger_void_function_)()= nullptr;
public:
    TimerRemoveSession(int, bool(*)());
    bool TimerTrigger() override;
};

//endregion

//region 函数原型声明
int EventNewGui();
ssize_t EventGuiMessage(FdInfo *client);
ssize_t EventServerMessage();
int EventStdinMessage();

void BaseActionInterrupt();
int ActionSendMessageToServer(const char*);
int ActionReceiveMessageFromServer(const char*);
int ActionChapResponse(const char*);
int ActionPrintConsoleHelp(const char*);
int ActionConnectToServer();
int ActionControlLogin();
int ActionControlLs();
int ActionControlMonitor();
int ActionFileRequestSend(const char*);
int ActionFileResponseReceived(const char * );
int ActionFileTransportingReceived(const char*);
int ActionFileAckSend(uint32_t, uint32_t);
int ActionFileErrorSend(uint32_t, uint32_t);
int ActionFileEndReceived(const char*);

bool ErrorSimulator(int);

//endregion

extern int conn_client_fd,gui_server_fd;
extern bool cue_flag;

extern fd_set * client_fd_set;
extern std::list<FdInfo>* gui_client_list;
extern std::list<FileSession>* file_list;
extern int max_fd;
extern struct ClientSelfData * self_data;

#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
