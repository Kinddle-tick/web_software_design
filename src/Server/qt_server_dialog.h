//
// Created by Kinddle Lee on 2021/12/26.
//

#ifndef WEB_SOFTWARE_DESIGN_FIVE_QT_SERVER_DIALOG_H
#define WEB_SOFTWARE_DESIGN_FIVE_QT_SERVER_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "../common_definition.h"
#include "server_socket_handle.h"

namespace QtServerDialog {
    QT_BEGIN_NAMESPACE
    namespace Ui { class QtServerDialog; }
    QT_END_NAMESPACE

//    class MyQListWidgetItem:public QObject,public QListWidgetItem{
//    Q_OBJECT
//        Q_PROPERTY(QString IP_ READ get_IP WRITE set_IP)
//        Q_PROPERTY(QString Port_ READ get_Port WRITE set_Port)
//    public:
//        explicit MyQListWidgetItem(const QListWidgetItem &other);
//        void set_IP(QString IP);
//        QString get_IP();
//        QString IP_;
//        void set_Port(QString Port);
//        QString get_Port();
//        QString Port_;
//
//    };
    struct ClientDetail{
        QString nickname;
        QString IP;
        QString Port;
        QString RootPath;
    };

    class QtServerDialog : public QDialog {
    Q_OBJECT

    public:
        explicit QtServerDialog(QWidget *parent = nullptr);
        ~QtServerDialog() override;

        void setDefaultInfo(const char* ip,uint16_t port);
        void MySetUp();
        void MyConnectDesign();
        static void setPushButtonEnable(QPushButton*button,bool enable);
    signals:
        //debug 控制相关
        void sig_LocalHint(const QString&);
        //控制线程中的循环计时器
        void sig_StartTimer();
        void sig_StopTimer();
        //同步信息到套接字
        void sig_SyncInfoWithSocket(const QString& ip, uint16_t port);
        //启动套接字
        void sig_EventServerLaunch();

    private:
        Ui::QtServerDialog *ui;
        ServerSocketThread *socket_thread_ = nullptr;
        ServerSocketHandle *socket_= nullptr;
        QList<ClientDetail> * client_detail_= nullptr;
    public slots:
        //界面按钮相关
        void slot_ButtonServerStart_clicked();
        void slot_ButtonServerShutdown_clicked();
        //界面提示框
        void slot_HintBrowser_show(const QString&);
        //界面客户端列表相关
        void slot_ClientListClear();
        void slot_ClientListAppend(const QString&,const QString&,const QString&,const QString&);
        void slot_ClientListItem_clicked(QListWidgetItem *);
        //套接字错误处理
        void slot_SocketErrorHandle(SocketErrorCode code);
    };
} // QtServerDialog

#endif //WEB_SOFTWARE_DESIGN_FIVE_QT_SERVER_DIALOG_H
