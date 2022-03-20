//
// Created by Kinddle Lee on 2021/12/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_qt_client_dialog.h" resolved

#include "qt_client_dialog.h"
#include "ui_qt_client_dialog.h"
#include "qt_client_file_handle_dialog.h"

//region #客户端界面常用的一些常量#
const char* kButtonDisableStyleSheet =  "background-color: #C5D1DF;"
                                        "color:#8b8b8b;"
                                        "border:2px groove gray;"
                                        "border-radius:6px;"
                                        "padding:5px 22px;";
const char* kButtonEnableStyleSheet = "background-color: #BBDEFB;"
                                      "color:#212121;"
                                      "border:2px groove gray;"
                                      "border-radius:6px;"
                                      "padding:5px 22px;";
const char* kDisplay_Header ="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                              "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
                              "p, li { white-space: pre-wrap; }\n"
                              "</style></head><body style=\" font-family:'.AppleSystemUIFont'; font-size:13pt; font-weight:400; font-style:normal;\">";
const char* kDisplay_footer ="</body></html>";
const char* kRightSideUsernameColor ="#aa7942";
const char* kRightSideTextColor ="color:#000000";
const char* kLeftSideUsernameColor ="#0433ff";
const char* kLeftSideTextColor ="color:#000000";

//endregion

namespace QtClientDialog {
    QtClientDialog::QtClientDialog(QWidget *parent) :
            QDialog(parent), ui_(new Ui::QtClientDialog) {
        ui_->setupUi(this);
        MySetUp();
        MyConnectDesign();
    }

    QtClientDialog::~QtClientDialog() {
        delete ui_;
    }

    void QtClientDialog::setPushButtonEnable(QPushButton *button, bool enable) {
        button->setEnabled(enable);
        if(enable){
            button->setStyleSheet(kButtonEnableStyleSheet);
        }
        else{
            button->setStyleSheet(kButtonDisableStyleSheet);
        }
    }

    void QtClientDialog::setDefaultInfo(const char *username, const char *password, const char *ip, uint16_t port) {
        ui_->UsernameEdit->setText(username);
        ui_->PasswordEdit->setText(password);
        ui_->IPEdit->setText(ip);
        QString tmp = QString::number(port);
        ui_->PortEdit->setText(tmp);
    }

    void QtClientDialog::MyConnectDesign() {
        connect(ui_->ButtonLink, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonLink_clicked);
        connect(ui_->ButtonShutdown, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonShutdown_clicked);
        connect(ui_->ButtonRegister, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonRegister_clicked);
        connect(ui_->ButtonLogin, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonLogin_clicked);
        connect(ui_->ButtonSend, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonSend_clicked);
        connect(ui_->ButtonFile, &QPushButton::clicked, this, &QtClientDialog::slot_ButtonFile_clicked);
        connect(this,&QtClientDialog::sig_LocalHint,this,&QtClientDialog::slot_HintBrowser_show);
        connect(this,&QtClientDialog::sig_DisplayEditLocalAppend,this,&QtClientDialog::slot_DisplayEdit_append);
        connect(this,&QtClientDialog::sig_LocalState,this,&QtClientDialog::slot_LabelUserState_show);
    }
    void QtClientDialog::MySetUp() {
        setPushButtonEnable(ui_->ButtonLink,true);
        setPushButtonEnable(ui_->ButtonShutdown,false);
        setPushButtonEnable(ui_->ButtonLogin,false);
        setPushButtonEnable(ui_->ButtonRegister,false);
        setPushButtonEnable(ui_->ButtonSend,false);
        setPushButtonEnable(ui_->ButtonCancle,false);
        setPushButtonEnable(ui_->ButtonFile,false);
        ui_->ClientListWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);//按住ctrl多选
        ui_->ClientListWidget->clear();
        ui_->DisplayEdit->clear();
        emit sig_LocalState(kOffline);
    }
    void QtClientDialog::slot_ButtonLink_clicked() {
        setPushButtonEnable(ui_->ButtonLink,false);
        setPushButtonEnable(ui_->ButtonShutdown,true);
        setPushButtonEnable(ui_->ButtonLogin, true);
        setPushButtonEnable(ui_->ButtonRegister,true);

        if(socket_ == nullptr){
            socket_ = new ClientSocketHandle();
            socket_thread_ = new ClientSocketThread(nullptr,socket_);
            connect(this, &QtClientDialog::sig_StartTimer, socket_thread_, &ClientSocketThread::slot_timer_ready);
            connect(this,&QtClientDialog::sig_StopTimer,socket_thread_,&ClientSocketThread::stop_timer);
            connect(this, &QtClientDialog::sig_SendMessageToServer, socket_, &ClientSocketHandle::slot_SendMessageToSomebody);
            connect(this, &QtClientDialog::sig_MissionFileToSocket, socket_, &ClientSocketHandle::slot_MissionFile);
            connect(socket_, &ClientSocketHandle::hint, this, &QtClientDialog::slot_HintBrowser_show);
            connect(socket_,&ClientSocketHandle::state,this,&QtClientDialog::slot_LabelUserState_show);
            connect(socket_,&ClientSocketHandle::message,this,&QtClientDialog::slot_DisplayEdit_append);
            connect(socket_,&ClientSocketHandle::sig_ClientListClear,this,&QtClientDialog::slot_ClientListClear);
            connect(socket_,&ClientSocketHandle::sig_ClientListAppend,this,&QtClientDialog::slot_ClientListAppend);
            connect(socket_,&ClientSocketHandle::sig_MenuCacheClientGet,this,&QtClientDialog::slot_MenuCacheClientGet);
            connect(socket_,&ClientSocketHandle::sig_MenuCacheServerGet,this,&QtClientDialog::slot_MenuCacheServerGet);

            connect(this,&QtClientDialog::sig_EventLogin,socket_,&ClientSocketHandle::slot_EventLogin);
            connect(this,&QtClientDialog::sig_EventLogout,socket_,&ClientSocketHandle::slot_EventLogout);
            connect(this,&QtClientDialog::sig_EventRegister,socket_,&ClientSocketHandle::slot_EventRegister);
            connect(this,&QtClientDialog::sig_MenuCacheFlush,socket_,&ClientSocketHandle::slot_MenuCacheFlush);

            connect(this, &QtClientDialog::sig_SyncInfoWithSocket, socket_, &ClientSocketHandle::slot_getInfoFromDialog);
            connect(this,&QtClientDialog::sig_EventConnectServer,socket_,&ClientSocketHandle::slot_EventConnectServer);
            connect(socket_,&ClientSocketHandle::sig_ErrorCodeSend,this,&QtClientDialog::slot_SocketErrorHandle);
            socket_thread_->start();
            emit sig_StartTimer();
            emit sig_SyncInfoWithSocket(ui_->UsernameEdit->text(),
                                        ui_->PasswordEdit->text(),
                                        ui_->IPEdit->text(),
                                        ui_->PortEdit->text().toInt());
            emit sig_EventConnectServer();
        }
        else{
            printf("不能吧...");
        }
    }
    void QtClientDialog::slot_ButtonShutdown_clicked() {
        setPushButtonEnable(ui_->ButtonLink,true);
        setPushButtonEnable(ui_->ButtonShutdown,false);
        setPushButtonEnable(ui_->ButtonLogin,false);
        setPushButtonEnable(ui_->ButtonRegister,false);
        ui_->ClientListWidget->clear();
        emit sig_StopTimer();
        delete socket_;
        socket_thread_->quit();
        socket_thread_->wait();
        socket_= nullptr;
        socket_thread_ = nullptr;
        emit sig_LocalState(kOffline);
        emit sig_LocalHint("[+]清理套接字完毕...");
    }
    void QtClientDialog::slot_ButtonLogin_clicked() {
        if(ui_->ButtonLogin->text()=="登录"){
            emit sig_LocalState(kHalfLink);
            emit sig_SyncInfoWithSocket(ui_->UsernameEdit->text(),
                                        ui_->PasswordEdit->text(),
                                        ui_->IPEdit->text(),
                                        ui_->PortEdit->text().toInt());
            emit sig_EventLogin();
        }
        else if(ui_->ButtonLogin->text()=="注销"){
//            emit sig_LocalState(kOnline);
            emit sig_EventLogout();
        }

    }
    void QtClientDialog::slot_ButtonRegister_clicked() {
        emit sig_SyncInfoWithSocket(ui_->UsernameEdit->text(),
                                    ui_->PasswordEdit->text(),
                                    ui_->IPEdit->text(),
                                    ui_->PortEdit->text().toInt());
        emit sig_EventRegister();
    }

    void QtClientDialog::slot_SocketErrorHandle(SocketErrorCode code) {
        switch (code) {
            case kSocketErrorConnectRefused:{
                emit ui_->ButtonShutdown->clicked();
                break;
            }
        }
    }
    void QtClientDialog::slot_HintBrowser_show(const QString& str) {
        QString tmp = ui_->HintBrowser->toPlainText();
        ui_->HintBrowser->document()->setMaximumBlockCount(4);
        ui_->HintBrowser->setText(tmp+"\n"+str);
    }

    void QtClientDialog::slot_ButtonSend_clicked() {
        emit sig_SyncInfoWithSocket(ui_->UsernameEdit->text(),
                                    ui_->PasswordEdit->text(),
                                    ui_->IPEdit->text(),
                                    ui_->PortEdit->text().toInt());
        auto client_selected_item_list = ui_->ClientListWidget->selectedItems();
        emit sig_DisplayEditLocalAppend(ui_->InputEdit->toPlainText(),ui_->UsernameEdit->text(),true);
        if(client_selected_item_list.empty()){
            emit sig_SendMessageToServer(ui_->InputEdit->toPlainText(), "");
        }
        for(auto &iter:client_selected_item_list){
            emit sig_SendMessageToServer(ui_->InputEdit->toPlainText(), iter->text());
        }
    }
    void QtClientDialog::slot_ButtonFile_clicked() {
//        FileWindow = nullptr;
        auto * w = new ClientFileHandleDialog::ClientFileHandleDialog(this);
//        ClientFileHandleDialog::ClientFileHandleDialog w(this);
        w->ClientSignalSlotConnect(this);
        FileWindow = w;
        FlushMenuCache();
//        w->exec();
    }

    void QtClientDialog::slot_LabelUserState_show(State state) {
        switch (state) {
            case kOffline:
                ui_->LabelUserState->setText("未链接");
                ui_->LabelUserState->setStyleSheet("background-color: rgb(245,245,245);color:#646464;");
                ui_->ButtonLogin->setText("登录");
                setPushButtonEnable(ui_->ButtonSend, false);
                setPushButtonEnable(ui_->ButtonFile, false);
                setPushButtonEnable(ui_->ButtonCancle, false);
                break;
            case kHalfLink:
                ui_->LabelUserState->setText("未登录");
                ui_->LabelUserState->setStyleSheet("background-color: rgb(245,245,245);color:#FF3300;");
                ui_->ButtonLogin->setText("登录");
                setPushButtonEnable(ui_->ButtonSend, false);
                setPushButtonEnable(ui_->ButtonFile, false);
                setPushButtonEnable(ui_->ButtonCancle, false);
                break;
            case kOnline:
                ui_->LabelUserState->setText("已登陆");
                ui_->LabelUserState->setStyleSheet("background-color: rgb(245,245,245);color:#00CC66;");
                ui_->ButtonLogin->setText("注销");
                setPushButtonEnable(ui_->ButtonSend, true);
                setPushButtonEnable(ui_->ButtonFile, true);
                setPushButtonEnable(ui_->ButtonCancle, true);
                break;
            default:
                emit slot_HintBrowser_show("未知的状态");
                break;
        }

    }

    void QtClientDialog::slot_ClientListClear() {
        ui_->ClientListWidget->clear();
    }

    void QtClientDialog::slot_ClientListAppend(const QString &nickname) {
        auto item = new QListWidgetItem(nickname,ui_->ClientListWidget);
        ui_->ClientListWidget->addItem(item);
    }

    void QtClientDialog::slot_DisplayEdit_append(const QString &msg, const QString &username, bool is_self) {
        static QString now_show ="";
        now_show = now_show+MakeLine(username,msg,is_self);
        ui_->DisplayEdit->setText(QString(kDisplay_Header)+now_show
                                  +QString(kDisplay_footer));
    }

    QString QtClientDialog::MakeLine(const QString &username, const QString &msg,bool right_side) {
        if(right_side){
            return  R"(<p align="right" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" color:)"
                    +QString(kRightSideUsernameColor)+";\">"+username+":    </span></p>\n"+
                    R"(<p align="right" style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" color:)"
                    +QString(kRightSideTextColor)+";\">"+msg+"    </span></p>\n";
        }
        else
            return R"(<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" color:)"
                    +QString(kLeftSideUsernameColor)+";\">"+username+":</span></p>\n"+
                    R"(<p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" color:)"
                    +QString(kLeftSideTextColor)+";\">"+msg+"</span></p>\n";

    }

    void QtClientDialog::FlushMenuCache() {
        MenuCacheClientFlushFlag = MenuCacheServerFlushFlag = false;
        emit sig_MenuCacheFlush();
    }

    void QtClientDialog::slot_Mission_File(const QString &server_path, const QString &client_path, bool upload,
                                           bool download) {
        if(upload){
            emit sig_LocalHint(client_path+" ➡ "+server_path);
        }
        if(download){
            emit sig_LocalHint(client_path+" ⬅ "+server_path);
        }
        emit sig_MissionFileToSocket(server_path,client_path,upload,download);
    }

    void QtClientDialog::slot_MenuCacheServerGet(const QString & server_path) {
        MenuCacheServerFlushFlag = true;
        emit sig_MenuCacheServer(server_path);
        if(MenuCacheClientFlushFlag){
            MenuCacheServerFlushFlag = MenuCacheClientFlushFlag = false;
            FileWindow->open();
//            FileWindow->exec();
        }
    }

    void QtClientDialog::slot_MenuCacheClientGet(const QString & client_path) {
        MenuCacheClientFlushFlag = true;
//        emit sig_LocalHint(client_path);
        emit sig_MenuCacheClient(client_path);

        if(MenuCacheServerFlushFlag){
            MenuCacheServerFlushFlag = MenuCacheClientFlushFlag = false;
            FileWindow->exec();
        }
    }


} // QtClientDialog
