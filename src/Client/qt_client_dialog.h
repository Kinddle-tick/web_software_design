//
// Created by Kinddle Lee on 2021/12/26.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_DIALOG_H
#define WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_DIALOG_H

#include <QDialog>
#include <QThread>
#include "../common_definition.h"
#include "client_socket_handle.h"
namespace QtClientDialog {
    QT_BEGIN_NAMESPACE
    namespace Ui { class QtClientDialog; }
    QT_END_NAMESPACE

    class QtClientDialog : public QDialog {
    Q_OBJECT

    public:
        explicit QtClientDialog(QWidget *parent = nullptr);
        ~QtClientDialog() override;

        void setDefaultInfo(const char* username,const char* password,const char* ip,uint16_t port);
        void MyConnectDesign();
        void MySetUp();
        static void setPushButtonEnable(QPushButton*button,bool enable);
        static QString MakeLine(const QString& username,const QString& msg,bool right_side);
        void FlushMenuCache();
        signals:
        void sig_MenuCacheFlush();
        void sig_MenuCacheServer(const QString&);
        void sig_MenuCacheClient(const QString&);
        //debug 控制相关
        void sig_LocalHint(const QString&);
        void sig_LocalState(State);
        //控制线程中的循环计时器
        void sig_StartTimer();
        void sig_StopTimer();
        //输出框相关
        void sig_DisplayEditLocalAppend(const QString& msg,const QString& username,bool is_self);
        //传送信息到服务器
        void sig_SendMessageToServer(const QString&str, const QString&who);//跨线程 不要用char*指针传递字符串
        //同步信息到套接字
        void sig_SyncInfoWithSocket(const QString& username, const QString& password, const QString& ip, uint16_t port);
        //文件对话框槽函数
        void sig_MissionFileToSocket(const QString& server_path, const QString& client_path,bool upload,bool download);
        //连接服务器
        void sig_EventConnectServer();
        //登录-注册-注销
        void sig_EventLogin();
        void sig_EventRegister();
        void sig_EventLogout();

    private:
        Ui::QtClientDialog *ui_;
        ClientSocketThread *socket_thread_= nullptr;
        ClientSocketHandle *socket_= nullptr;
        bool MenuCacheServerFlushFlag = false,MenuCacheClientFlushFlag=false;
        QString MenuCacheServer="";
        QString MenuCacheClient="";
        QDialog * FileWindow;

    public slots:
        //界面按钮相关
        void slot_ButtonLink_clicked();
        void slot_ButtonShutdown_clicked();
        void slot_ButtonLogin_clicked();
        void slot_ButtonRegister_clicked();
        void slot_ButtonSend_clicked();
        void slot_ButtonFile_clicked();
        //文件对话框槽函数
        void slot_Mission_File(const QString& server_path, const QString& client_path,bool upload,bool download);
        //界面提示框相关
        void slot_HintBrowser_show(const QString&);
        //界面输出框相关
        void slot_DisplayEdit_append(const QString& msg,const QString& username,bool is_self=false);
        //当前状态相关
        void slot_LabelUserState_show(State);
        //客户端列表相关
        void slot_ClientListClear();
        void slot_ClientListAppend(const QString&);
        //套接字的错误处理
        void slot_SocketErrorHandle(SocketErrorCode code);
        void slot_MenuCacheServerGet(const QString&);
        void slot_MenuCacheClientGet(const QString&);
    };







} // QtClientDialog

#endif //WEB_SOFTWARE_DESIGN_FIVE_QT_CLIENT_DIALOG_H
