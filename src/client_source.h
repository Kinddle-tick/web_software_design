//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
#include "my_generic_definition.h"
#include <list>

//region 宏定义与枚举类型
#define CLIENT_DEBUG_LEVEL 0
//0: 没有debug信息
//3：对定时器相关的信息进行展示
//5: 增加进入到部分事件的内容
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
    timespec init_time;
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
    timespec time_interval_{};
    timespec init_tick_{};
//    int timing_second_;

    int retry_max_=10;
protected:
    State timer_state_;
public:
    uint8_t timer_id_;
    SocketFileDescriptor socket_fd_;
    explicit TimerSession(double_t ,uint8_t,SocketFileDescriptor);
    bool TimerUpdate();
    bool TimeoutJustice() const;
    void set_timing_interval(double_t time_interval_);
    void set_retry_max(int);
    double_t get_timing_interval() const;
    virtual bool TimerTrigger();
    bool TimerDisable();
    State get_timer_state_();
};

class TimerRetranslationSession:public TimerSession{
private:
    char packet_cache_[kHeaderSize+kBufferSize]{0};
    SocketFileDescriptor client_fd_;
    size_t packet_length_;
public:
    TimerRetranslationSession(double_t ,uint8_t ,SocketFileDescriptor,const char*,size_t);
    bool TimerTrigger() override;
};

class TimerRemoveSession:public TimerSession{
private:
    bool (* trigger_void_function_)()= nullptr;
public:
    TimerRemoveSession(double_t ,uint8_t ,SocketFileDescriptor,bool(*)());
    bool TimerTrigger() override;
};

//endregion

//region 函数原型声明
int EventNewGui();
ssize_t EventGuiMessage(FdInfo *client);
ssize_t EventServerMessage();
int EventStdinMessage();

int ActionGeneralFinishSend(const char *);
int ActionGeneralFinishReceive(const char * );
int ActionGeneralAckSend(const char *);
int ActionGeneralAckReceive(const char *);

void BaseActionInterrupt();
int ActionSendMessageToServer(const char*,const char*);
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

bool TimerDisable(uint8_t,SocketFileDescriptor);
//endregion

extern int conn_client_fd,gui_server_fd;
extern bool cue_flag;
extern uint8_t client_timer_id;

extern fd_set * client_fd_set;
extern std::list<FdInfo>* gui_client_list;
extern std::list<FileSession>* file_list;
extern std::list<TimerSession*>* timer_list;
extern int max_fd;
extern struct ClientSelfData * self_data;

#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_CLIENT_SOURCE_H
