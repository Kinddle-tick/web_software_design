//
// Created by Kinddle Lee on 2021/12/27.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_SERVER_SOCKET_HANDLE_H
#define WEB_SOFTWARE_DESIGN_FIVE_SERVER_SOCKET_HANDLE_H
#include <QObject>
#include <QThread>
#include <QTimer>
#include "../common_definition.h"

class TimerSession;

struct ClientSession{
    SocketFileDescriptor socket_fd = 0;
    uint8_t server_timer_id =0;
    State state = kOffline;
    timespec tick{};
    UserNameString nickname = "";
    QString ip_info ="";
    QString port_info = "";
    char now_path[kUserPathMaxLength]={0};
};
struct ChapSession{
    uint32_t sequence;
    uint32_t answer;
    ClientSession * client;
    timespec tick;
};
struct UserInfo{
    UserNameString user_name;
    unsigned int password;
    char root_path[kUserPathMaxLength];
};
struct FileSession{
    uint32_t session_id = 0;
    uint32_t sequence = 0;
    FileDescriptor file_fd=-1;
    State state =kFileInit;
    timespec init_time{};
    uint32_t init_sequence = 0;
    ClientSession * client = nullptr;
    QTimer * next_timer= nullptr;
    int delay=100;
};

class ServerSocketHandle:public QObject{
Q_OBJECT
private:
//    QString username_;
//    QString password_;
    QString ip_;
    uint16_t port_ = -1;

    fd_set * server_fd_set_= nullptr;
    uint16_t max_fd_=1;
    struct timeval ctl_time_{0,200000};//0.2s一轮
    int conn_server_fd_ = -1;
    QList<ClientSession> *client_list_;
    QList<TimerSession*> *timer_list_;
    QList<UserInfo> *  registered_users_list_;
    QList<ChapSession>* chap_list_;
    QList<FileSession>* server_file_list_;
public:
    explicit ServerSocketHandle(QObject* parent = nullptr);
    ~ServerSocketHandle() override;
    void MyQtConnection();
    QString step_MenuCacheFlush(DIR* ,int deep, const QString &);
    bool GeneralSend(const char* received_packet_total,char* send_packet_total,ClientSession*client,int retry_time = 10);
signals:
    //debug 控制相关
    void sig_mission_finish(bool);
    void sig_ErrorCodeSend(SocketErrorCode code);
    void sig_TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd);
    //界面内容控制
    void hint(const QString&);
    void sig_FlushClientList();
    void sig_ClientListClear();
    void sig_ClientListAppend(const QString&,const QString&,const QString&,const QString&);
    //获得存储的数据
    void sig_CreateStorageEnv();
    void sig_SaveStorageEnv();
    //客户端的建立和报文处理
    void sig_EventNewClient();
    void sig_MainEventClientPacket(ClientSession* client);
    //通用的结束方法
    void sig_ActionGeneralFinishSend(const char *, ClientSession *);
    void sig_ActionGeneralFinishReceive(const char *, ClientSession *);
    void sig_ActionGeneralAckSend(const char *, ClientSession *);
    void sig_ActionGeneralAckReceive(const char *, ClientSession *);
    //处理收到的信息
    void sig_ActionMessageProcessing(const char *, ClientSession *);
    //服务器文件列表
    void sig_ActionControlServerFileLsReceive(const char *, ClientSession *);
    //登录相关
    void sig_ActionControlLogin(const char * receive_packet_total, ClientSession * client);
    void sig_ActionControlUnregistered(const char *receive_packet_total,ClientSession* client);
    void sig_ActionControlDuplicateLogin(const char *receive_packet_total,ClientSession* client);
    //注册相关
    void sig_ActionControlRegister(const char * receive_packet_total, ClientSession * client);
    void sig_ActionControlRegisterSuccess(const char * receive_packet_total, ClientSession * client);
    void sig_ActionControlDuplicateRegister(const char * receive_packet_total, ClientSession * client);
    //注销相关
    void sig_ActionControlLogout(const char * receive_packet_total, ClientSession * client);
    void sig_ActionControlLogoutAck(const char * receive_packet_total, ClientSession * client);
    //分发在线用户表单
    void sig_ActionControlClientInfoSend(const char *receive_packet_total,ClientSession* client);
    //CHAP质询相关
    void sig_ActionChapChallenge(const char *receive_packet_total,ClientSession* client);
    void sig_ActionChapJustice(const char* receive_packet_total, ClientSession* client);
    //文件相关
    void sig_ActionFileUpload(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileDownload(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileUploadReceive(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileDownloadReceive(const char* receive_packet_total, ClientSession* client);
//    void sig_ActionFileResponseSend(const char* receive_packet_total, ClientSession* client);
//    void sig_ActionFileResponseReceive(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileTranslatingSend(FileSession * file_session, ClientSession* client);
    void sig_ActionFileTranslatingReceive(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileAckSend(const char* receive_packet_total,ssize_t offset,FileSession * file_session, ClientSession* client);
    void sig_ActionFileAckReceived(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileEndSend(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileEndReceived(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileErrorSend(const char* receive_packet_total, ClientSession* client);
    void sig_ActionFileErrorReceived(const char* receive_packet_total, ClientSession* client);
    //队列处理 - 客户端清理，计时器清理，客户端表单泛洪
    void sig_QueueActionCleanClientList();//队列处理
    void sig_QueueActionCleanTimerList();
    void sig_QueueSpontaneousClientInfoFlood();

public slots:
    //主循环
    void slot_timeout();
    //通用的结束方法
    void slot_ActionGeneralFinishSend(const char *, ClientSession *);
    void slot_ActionGeneralFinishReceive(const char *, ClientSession *);
    void slot_ActionGeneralAckSend(const char *, ClientSession *);
    void slot_ActionGeneralAckReceive(const char *, ClientSession *);
    //与主对话框有关
    void slot_FlushClientList();//刷新客户端列表
    void slot_getInfoFromDialog(const QString& ip,uint16_t port);
    void slot_EventServerLaunch();//准备接受链接
    //获得存储的数据
    void slot_SaveStorageEnv();
    void slot_CreateStorageEnv();
    //计时器控制
    void slot_TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd);
    //处理收到的客户端发来的包以及建立新的客户端链接
    void slot_MainEventClientPacket(ClientSession* client);
    void slot_EventNewClient();
    //服务器文件列表
    void slot_ActionControlServerFileLsReceive(const char *, ClientSession *);
    //信息处理
    void slot_ActionMessageProcessing(const char * receive_packet_total, ClientSession * client);
    //登录相关
    void slot_ActionControlLogin(const char * receive_packet_total, ClientSession * client);
    void slot_ActionControlUnregistered(const char *receive_packet_total,ClientSession* client);
    void slot_ActionControlDuplicateLogin(const char *receive_packet_total,ClientSession* client);
    //注册相关
    void slot_ActionControlRegister(const char * receive_packet_total, ClientSession * client);
    void slot_ActionControlRegisterSuccess(const char * receive_packet_total, ClientSession * client);
    void slot_ActionControlDuplicateRegister(const char * receive_packet_total, ClientSession * client);
    //注销相关
    void slot_ActionControlLogout(const char * receive_packet_total, ClientSession * client);
    void slot_ActionLogoutAck(const char * receive_packet_total, ClientSession * client);
    //分发在线用户表单
    void slot_ActionControlClientInfoSend(const char *receive_packet_total,ClientSession* client);
    //质询相关
    void slot_ActionChapChallenge(const char *receive_packet_total,ClientSession* client);
    void slot_ActionChapJustice(const char* receive_packet_total, ClientSession* client);
    //文件相关
    void slot_ActionFileUploadSend(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileDownloadSend(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileUploadReceive(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileDownloadReceive(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileTranslatingSend(FileSession * file_session, ClientSession* client);
    void slot_ActionFileTranslatingReceive(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileAckSend(const char* receive_packet_total,ssize_t offset,FileSession * file_session,ClientSession* client);
    void slot_ActionFileAckReceived(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileEndSend(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileEndReceived(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileErrorSend(const char* receive_packet_total, ClientSession* client);
    void slot_ActionFileErrorReceived(const char* receive_packet_total, ClientSession* client);
    //队列处理 - 客户端清理，计时器清理，客户端表单泛洪
    void slot_QueueActionCleanClientList();//队列处理
    void slot_QueueActionCleanTimerList();
    void slot_QueueSpontaneousClientInfoFlood();
};

class ServerSocketThread:public QThread{
Q_OBJECT
private:
    bool flag_timer_running_= false;
    QTimer *timer_;
    ServerSocketHandle *server_socket_handle_;
public:
    explicit ServerSocketThread(QThread* parent=nullptr,ServerSocketHandle* =nullptr);
    ~ServerSocketThread() override;

protected:
    void run() override;

public slots:
    void stop_timer();
    void slot_timer_ready();
    void slot_turn_end(bool);
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

#endif //WEB_SOFTWARE_DESIGN_FIVE_SERVER_SOCKET_HANDLE_H
