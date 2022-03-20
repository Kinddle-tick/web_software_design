//
// Created by Kinddle Lee on 2021/12/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_qt_server_dialog.h" resolved

#include "qt_server_dialog.h"
#include "ui_qt_server_dialog.h"

//region #服务器界面常用的一些常量#
const char* kButtonDisableStyleSheet = "background-color: #C5D1DF;"
                                       "color:#8b8b8b;"
                                       "border:2px groove gray;"
                                       "border-radius:6px;"
                                       "padding:5px 22px;";
const char* kButtonEnableStyleSheet = "background-color: #BBDEFB;"
                                      "color:#212121;"
                                      "border:2px groove gray;"
                                      "border-radius:6px;"
                                      "padding:5px 22px;";
//endregion

namespace QtServerDialog {
    QtServerDialog::QtServerDialog(QWidget *parent) :
            QDialog(parent), ui(new Ui::QtServerDialog) {
        ui->setupUi(this);
        MySetUp();
        MyConnectDesign();
        client_detail_ = new QList<ClientDetail>;
    }

    QtServerDialog::~QtServerDialog() {
        delete ui;
        delete client_detail_;
    }

    void QtServerDialog::setDefaultInfo(const char *ip, uint16_t port) {
        ui->ServerIPEdit->setText(ip);
        QString tmp = QString::number(port);
        ui->ServerPortEdit->setText(tmp);
    }

    void QtServerDialog::MySetUp() {
        setPushButtonEnable(ui->ButtonServerShutdown, false);
        setPushButtonEnable(ui->ButtonServerStart, true);
        setPushButtonEnable(ui->ButtonClientClose, false);
        setPushButtonEnable(ui->ButtonClientFileBase, false);
    }

    void QtServerDialog::MyConnectDesign() {
        connect(ui->ButtonServerStart,&QPushButton::clicked,this,&QtServerDialog::slot_ButtonServerStart_clicked);
        connect(ui->ButtonServerShutdown,&QPushButton::clicked,this,&QtServerDialog::slot_ButtonServerShutdown_clicked);
        connect(this,&QtServerDialog::sig_LocalHint,this,&QtServerDialog::slot_HintBrowser_show);
        connect(ui->ClientList,&QListWidget::itemClicked,this,&QtServerDialog::slot_ClientListItem_clicked);
    }

    void QtServerDialog::setPushButtonEnable(QPushButton *button, bool enable) {
        button->setEnabled(enable);
        if(enable){
            button->setStyleSheet(kButtonEnableStyleSheet);
            button->setCursor(Qt::ArrowCursor);
        }
        else{
            button->setStyleSheet(kButtonDisableStyleSheet);
            button->setCursor(Qt::ForbiddenCursor);
        }
    }



    void QtServerDialog::slot_ButtonServerStart_clicked() {
        setPushButtonEnable(ui->ButtonServerStart,false);
        setPushButtonEnable(ui->ButtonServerShutdown, true);

        if(socket_ == nullptr){
            socket_ = new ServerSocketHandle(this);
            socket_thread_ = new ServerSocketThread(nullptr, socket_);
            connect(this,&QtServerDialog::sig_StartTimer,socket_thread_,&ServerSocketThread::slot_timer_ready);
            connect(this, &QtServerDialog::sig_SyncInfoWithSocket, socket_, &ServerSocketHandle::slot_getInfoFromDialog);
            connect(socket_,&ServerSocketHandle::hint,this,&QtServerDialog::slot_HintBrowser_show);
            connect(socket_,&ServerSocketHandle::sig_ErrorCodeSend,this,&QtServerDialog::slot_SocketErrorHandle);
            connect(socket_,&ServerSocketHandle::sig_ClientListAppend,this,&QtServerDialog::slot_ClientListAppend);
            connect(socket_,&ServerSocketHandle::sig_ClientListClear,this,&QtServerDialog::slot_ClientListClear);
//            connect(socket_,&ServerSocketHandle::sig_FlushClientList,this,&QtServerDialog::slot_FlushClientList);
            connect(this,&QtServerDialog::sig_EventServerLaunch,socket_,&ServerSocketHandle::slot_EventServerLaunch);
            connect(this,&QtServerDialog::sig_StopTimer,socket_thread_,&ServerSocketThread::stop_timer);

            socket_thread_->start();
            emit sig_SyncInfoWithSocket(ui->ServerIPEdit->text(),
                                        ui->ServerPortEdit->text().toInt());
            emit sig_EventServerLaunch();
            emit sig_StartTimer();
        }
        else{
            printf("不应该啊...");
        }
    }

    void QtServerDialog::slot_ButtonServerShutdown_clicked() {
        setPushButtonEnable(ui->ButtonServerStart,true);
        setPushButtonEnable(ui->ButtonServerShutdown, false);

        emit sig_StopTimer();
        delete socket_;
        socket_thread_->quit();
        socket_thread_->wait();
        socket_= nullptr;
        socket_thread_ = nullptr;
        emit sig_LocalHint("服务器断开");
    }

    void QtServerDialog::slot_HintBrowser_show(const QString& str) {
        QString tmp = ui->HintBrowser->toPlainText();
        ui->HintBrowser->document()->setMaximumBlockCount(4);
        ui->HintBrowser->setText(tmp+"\n"+str);
    }

    void QtServerDialog::slot_ClientListClear() {
        ui->ClientList->clear();
        client_detail_->clear();
    }

    void QtServerDialog::slot_ClientListAppend(const QString& nickname,const QString& ip,const QString& port,const QString& root_path) {
        auto item = new QListWidgetItem(nickname,ui->ClientList);
        auto client_tmp = ClientDetail{.nickname =nickname,
                                       .IP = ip,
                                       .Port = port,
                                       .RootPath = root_path};
        client_detail_->push_back(client_tmp);
        ui->ClientList->addItem(item);
    }

    void QtServerDialog::slot_ClientListItem_clicked(QListWidgetItem *item) {
        for(const auto& iter : *client_detail_){
            if(iter.nickname == item->text()){
                ui->ClientIPShow->setText(iter.IP);
                ui->ClientPortShow->setText(iter.Port);
                emit sig_LocalHint(iter.RootPath);
                return;
            }
        }
        emit sig_LocalHint("未找到相关信息");
    }

    void QtServerDialog::slot_SocketErrorHandle(SocketErrorCode code) {
        switch (code) {
            case kSocketErrorConnectRefused:{
                emit ui->ButtonServerShutdown->clicked();
                break;
            }
        }
    }

} // QtServerDialog
