//
// Created by Kinddle Lee on 2021/12/27.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_CLIENT_SOCKET_HANDLE_H
#define WEB_SOFTWARE_DESIGN_FIVE_CLIENT_SOCKET_HANDLE_H
#include <QObject>
#include <QThread>
#include <QTimer>
#include "../common_definition.h"
class TimerSession;
const int kDirLength = 30; //请注意根据文件夹的名称更改此数字
const char client_data_root[kDirLength] = "client_data/";
struct ClientSelfData{
    UserNameString confirmUserName = "";
    uint32_t password = 0;
    State state = kOffline;
    char client_data_dir_now[kDirLength];
};
struct FileSession{
    uint32_t session_id = 0;
    uint32_t sequence = 0;
    FileDescriptor file_fd=-1;
    State state =kFileInit;
    timespec init_time{};
    uint32_t init_sequence = 0;
    QTimer * next_timer= nullptr;
    int delay=100;
};


class ClientSocketHandle:public QObject{
    Q_OBJECT
private:
    QString username_;
    QString password_;
    QString ip_;
    uint16_t port_ = 0;
    fd_set * client_fd_set_= nullptr;
    uint16_t max_fd_=1;
    struct timeval ctl_time_{0,200000};//0.2s一轮
    int conn_client_fd_ = -1;
    ClientSelfData *self_data_;
    uint8_t client_timer_id_=0;
    uint32_t session_id_;
    QList<TimerSession*> *timer_list_;
    QList<FileSession>* client_file_list_;

public:
    explicit ClientSocketHandle(QObject* parent = nullptr);
    ~ClientSocketHandle() override;
    void MyQtConnection();
    bool GeneralSend(const char* received_packet_total,char* send_packet_total,int retry_time=10);
    QString step_MenuCacheFlush(DIR* ,int deep, const QString &);
    signals:
    //debug 控制相关
    void sig_mission_finish(bool);
    void sig_TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd);
    void sig_ErrorCodeSend(SocketErrorCode code);
    //界面内容控制
    void hint(const QString&);
    void state(State);
    void message(const QString& msg,const QString& username,bool is_self=false);
    void sig_ClientListClear();
    void sig_ClientListAppend(const QString&);
    void sig_MenuCacheServerGet(const QString&);
    void sig_MenuCacheClientGet(const QString&);
    //存储文件相关
    void sig_ClientStorageConfirm();
    //报文处理
    void sig_MainEventServerPacket();
    //信息处理
    void sig_EventSendMessageToServer(const QString &message,const QString &username="");
    void sig_EventReceiveMessageFromServer(const char *receive_packet_total);
    //服务器关机处理
    void sig_EventServerShutdown();
    //获取服务器文件内容
    void sig_ActionControlServerFileLsSend();
    void sig_ActionControlServerFileLsReceive(const char *);
    //通用结束方法
    void sig_ActionGeneralFinishSend(const char *);
    void sig_ActionGeneralFinishReceive(const char *);
    void sig_ActionGeneralAckSend(const char *);
    void sig_ActionGeneralAckReceive(const char *);
    //接受在线用户表单
    void sig_ActionControlClientInfoReceive(const char *receive_packet_total);
    //CHAP质询相关
    void sig_ActionChapResponse(const char* receive_packet_total);
    //文件相关
    void sig_ActionFileUploadSend(const char* file_path_server, const char* file_path_client);
    void sig_ActionFileDownloadSend(const char* file_path_server, const char* file_path_client);
    void sig_ActionFileUploadReceive(const char* receive_packet_total);
    void sig_ActionFileDownloadReceive(const char* receive_packet_total);
//    void sig_ActionFileResponseSend(const char* receive_packet_total);
//    void sig_ActionFileResponseReceive(const char* receive_packet_total);
    void sig_ActionFileTranslatingSend(const char* receive_packet_total);
    void sig_ActionFileTranslatingReceive(const char* receive_packet_total);
    void sig_ActionFileAckSend(const char* receive_packet_total, ssize_t offset,FileSession * file_session);
    void sig_ActionFileAckReceived(const char* receive_packet_total);
    void sig_ActionFileEndSend(const char* receive_packet_total);
    void sig_ActionFileEndReceived(const char* receive_packet_total);
    void sig_ActionFileErrorSend(const char* receive_packet_total);
    void sig_ActionFileErrorReceived(const char* receive_packet_total);
    //队列处理 计时器清理
    void sig_QueueActionCleanTimerList();

public slots:
    //主循环
    void slot_timeout();
    //文件对话框槽函数
    void slot_MissionFile(const QString& server_path, const QString& client_path,bool upload,bool download);

    //通用的结束方法
    void slot_ActionGeneralFinishSend(const char *);
    void slot_ActionGeneralFinishReceive(const char  *);
    void slot_ActionGeneralAckSend(const char  *);
    void slot_ActionGeneralAckReceive(const char  *);
    //与主对话框有关
    void slot_getInfoFromDialog(const QString& username,const QString& password,const QString& ip,uint16_t port);
    //储存控件相关
    void slot_ClientStorageConfirm();
    //计时器控制
    void slot_TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd);
    //连接到服务器
    void slot_EventConnectServer();
    //处理收到的包
    void slot_MainEventServerPacket();
    //信息处理
    void slot_EventSendMessageToServer(const QString &message,const QString &username="");
    void slot_SendMessageToSomebody(const QString &str,const QString &who);
    void slot_EventReceiveMessageFromServer(const char *receive_packet_total);
    //服务器关机处理
    void slot_EventServerShutdown();
    //获取服务器文件列表
    void slot_ActionControlServerFileLsSend();
    void slot_ActionControlServerFileLsReceive(const char *);
    //登录-注册-注销
    void slot_EventLogin();
    void slot_EventRegister();
    void slot_EventLogout();
    //收到在线用户表单
    void slot_ActionControlClientInfoReceive(const char *receive_packet_total);
    //质询相关
    void slot_ActionChapResponse(const char* receive_packet_total);
    //文件相关
    void slot_ActionFileUploadSend(const char* file_path_server, const char* file_path_client);
    void slot_ActionFileDownloadSend(const char* file_path_server, const char* file_path_client);
    void slot_ActionFileUploadReceive(const char* receive_packet_total);
    void slot_ActionFileDownloadReceive(const char* receive_packet_total);
    void slot_ActionFileTranslatingSend(const char* receive_packet_total);
    void slot_ActionFileTranslatingReceive(const char* receive_packet_total);
    void slot_ActionFileAckSend(const char* receive_packet_total, ssize_t offset,FileSession * file_session);
    void slot_ActionFileAckReceived(const char* receive_packet_total);
    void slot_ActionFileEndSend(const char* receive_packet_total);
    void slot_ActionFileEndReceived(const char* receive_packet_total);
    void slot_ActionFileErrorSend(const char* receive_packet_total);
    void slot_ActionFileErrorReceived(const char* receive_packet_total);

    //队列 - 计时器清理
    void slot_QueueActionCleanTimerList();
    void slot_MenuCacheFlush();
};

class ClientSocketThread:public QThread{
    Q_OBJECT
private:
    bool flag_timer_running_= false;
    QTimer *timer_;
    ClientSocketHandle *client_socket_handle_;
public:
    explicit ClientSocketThread(QThread* parent=nullptr,ClientSocketHandle* =nullptr);
    ~ClientSocketThread() override;

protected:
    void run() override;

public slots:
    void slot_timer_ready();
    void stop_timer();
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

#endif //WEB_SOFTWARE_DESIGN_FIVE_CLIENT_SOCKET_HANDLE_H
