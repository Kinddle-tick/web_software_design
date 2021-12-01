//
// Created by Kinddle Lee on 2021/11/27.
//

#ifndef WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#define WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
#include "my_generic_definition.h"
#include <list>
//region 宏定义
#define SERVER_DEBUG_LEVEL 0
//0: 没有debug信息
//3：对定时器相关的信息进行展示

//#define kBacklog 16
//#define kDirLength 30 //请注意根据文件夹的名称更改此数字

const int kBacklog = 16;
const int  kDirLength = 30;//请注意根据文件夹的名称更改此数字
//endregion

//region 结构体声明
struct ClientSession{
    SocketFileDescriptor socket_fd = 0;
    uint8_t server_timer_id =0;
    State state = kOffline;
    timespec tick{};
    UserNameString nickname = "";
    char now_path[kUserPathMaxLength]={0};
};
struct UserInfo{
    UserNameString user_name;
    unsigned int password;
    char root_path[kUserPathMaxLength];
};
struct ChapSession{
    uint32_t sequence;
    uint32_t answer;
    ClientSession * client;
    timespec tick;
};
struct FileSession{
    uint32_t session_id;
    uint32_t sequence;
    FileDescriptor file_fd;
    FILE * file_ptr;
    State state;
    timespec tick;
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
int EventReceiveStdin();
int EventNewClient();
int EventClientMessage(ClientSession *client);

int ActionGeneralFinishSend(const char *,ClientSession *);
int ActionGeneralFinishReceive(const char *,ClientSession *);
int ActionGeneralAckSend(const char *,ClientSession *);
int ActionGeneralAckReceive(const char *,ClientSession *);

unsigned int ActionControlUnregistered(const char *,ClientSession *);
unsigned int ActionChapChallenge(const char *,ClientSession *);
unsigned int ActionChapJustice(const char *, ClientSession *);
int ActionControlLogin(const char*, ClientSession*);
int ActionControlLsResponse(const char*, ClientSession*);
int ActionFileResponse(const char*, ClientSession*);
int ActionFileTranslating(FileSession*, ClientSession*);
int ActionFileAckReceived(const char*, ClientSession*);
int ActionFileEndSend(const char*, ClientSession*);
int ActionMessageProcessing(const char *, ClientSession *);

ssize_t TimeoutActionRetransmission(const char * , ClientSession *);

bool ErrorSimulator(int);

bool TimerDisable(uint8_t,SocketFileDescriptor);
//endregion

extern int conn_server_fd;
extern unsigned int session_id;
extern std::list<ClientSession>* client_list;
extern std::list<ChapSession>* chap_list;
extern std::list<UserInfo>* user_list;
extern std::list<FileSession>* file_list;
extern std::list<TimerSession*>* timer_list;
#endif //WEB_SOFTWARE_DESIGN_COURSE_DESIGN_SERVER_SOURCE_H
