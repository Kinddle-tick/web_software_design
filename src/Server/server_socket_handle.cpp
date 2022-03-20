//
// Created by Kinddle Lee on 2021/12/27.
//

#include "server_socket_handle.h"
//#include <_fd_set.h>
using std::string;
using std::cout;
using std::endl;

const int kBacklog = 16;
const int  kDirLength = 30;//请注意根据文件夹的名称更改此数字
const char server_data_dir[kDirLength] = "server_data/";
//const char* kDatabaseDir ="/Users/kinddlelee/CLionProjects/web_software_design_five/database";

bool ErrorSimulator(int prob=0){
    if(random()%100 < prob){
        return true;
    }
    else{
        return false;
    }
}

ServerSocketHandle::ServerSocketHandle(QObject *parent) : QObject(parent) {
    server_fd_set_ = new fd_set;
    client_list_ = new QList<ClientSession>;
    timer_list_ = new QList<TimerSession*>;
    registered_users_list_ = new QList<UserInfo>;
    chap_list_ = new QList<ChapSession>;
    server_file_list_ = new QList<FileSession>;
    MyQtConnection();
    emit sig_CreateStorageEnv();
}
ServerSocketHandle::~ServerSocketHandle(){
    emit sig_SaveStorageEnv();
    if(conn_server_fd_!=-1){
        close(conn_server_fd_);
    }
//    disconnect(this,0,0,0);
    delete client_list_;
    delete timer_list_;
    delete server_fd_set_;
    delete registered_users_list_;
    delete chap_list_;
    delete server_file_list_;
//    printf("析构");
}

void ServerSocketHandle::MyQtConnection() {
    connect(this,&ServerSocketHandle::sig_EventNewClient,
            this,&ServerSocketHandle::slot_EventNewClient,Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_MainEventClientPacket,
            this, &ServerSocketHandle::slot_MainEventClientPacket, Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionMessageProcessing,
            this,&ServerSocketHandle::slot_ActionMessageProcessing,Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_SaveStorageEnv,
            this, &ServerSocketHandle::slot_SaveStorageEnv, Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_CreateStorageEnv,
            this,&ServerSocketHandle::slot_CreateStorageEnv,Qt::DirectConnection);
//    connect(this,&ServerSocketHandle::sig_ActionControlServerFileLsSend,
//            this,&ServerSocketHandle::slot_ActionControlServerFileLsSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlServerFileLsReceive,
            this,&ServerSocketHandle::slot_ActionControlServerFileLsReceive,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlLogin,
            this,&ServerSocketHandle::slot_ActionControlLogin,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlRegister,
            this,&ServerSocketHandle::slot_ActionControlRegister,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlRegisterSuccess,
            this,&ServerSocketHandle::slot_ActionControlRegisterSuccess,Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_ActionControlDuplicateRegister,
            this, &ServerSocketHandle::slot_ActionControlDuplicateRegister, Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlLogout,
            this,&ServerSocketHandle::slot_ActionControlLogout,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionChapChallenge,
            this,&ServerSocketHandle::slot_ActionChapChallenge,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlUnregistered,
            this,&ServerSocketHandle::slot_ActionControlUnregistered,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlDuplicateLogin,
            this,&ServerSocketHandle::slot_ActionControlDuplicateLogin,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionControlClientInfoSend,
            this,&ServerSocketHandle::slot_ActionControlClientInfoSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionChapJustice,
            this,&ServerSocketHandle::slot_ActionChapJustice,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_FlushClientList,
            this,&ServerSocketHandle::slot_FlushClientList,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionGeneralAckReceive,
            this,&ServerSocketHandle::slot_ActionGeneralAckReceive,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionGeneralAckSend,
            this,&ServerSocketHandle::slot_ActionGeneralAckSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionGeneralFinishReceive,
            this,&ServerSocketHandle::slot_ActionGeneralFinishReceive,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionGeneralFinishSend,
            this,&ServerSocketHandle::slot_ActionGeneralFinishSend,Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_ActionControlLogoutAck,
            this,&ServerSocketHandle::slot_ActionLogoutAck,Qt::DirectConnection);

    connect(this, &ServerSocketHandle::sig_ActionFileUpload,
            this, &ServerSocketHandle::slot_ActionFileUploadSend, Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_ActionFileDownload,
            this, &ServerSocketHandle::slot_ActionFileDownloadSend, Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_ActionFileUploadReceive,
            this, &ServerSocketHandle::slot_ActionFileUploadReceive, Qt::DirectConnection);
    connect(this, &ServerSocketHandle::sig_ActionFileDownloadReceive,
            this, &ServerSocketHandle::slot_ActionFileDownloadReceive, Qt::DirectConnection);
//    connect(this,&ServerSocketHandle::sig_ActionFileResponseSend,
//            this,&ServerSocketHandle::slot_ActionFileResponseSend,Qt::DirectConnection);
//    connect(this,&ServerSocketHandle::sig_ActionFileResponseReceive,
//            this,&ServerSocketHandle::slot_ActionFileResponseReceive,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileTranslatingSend,
            this,&ServerSocketHandle::slot_ActionFileTranslatingSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileTranslatingReceive,
            this,&ServerSocketHandle::slot_ActionFileTranslatingReceive,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileAckSend,
            this,&ServerSocketHandle::slot_ActionFileAckSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileAckReceived,
            this,&ServerSocketHandle::slot_ActionFileAckReceived,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileEndSend,
            this,&ServerSocketHandle::slot_ActionFileEndSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileEndReceived,
            this,&ServerSocketHandle::slot_ActionFileEndReceived,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileErrorSend,
            this,&ServerSocketHandle::slot_ActionFileErrorSend,Qt::DirectConnection);
    connect(this,&ServerSocketHandle::sig_ActionFileErrorReceived,
            this,&ServerSocketHandle::slot_ActionFileErrorReceived,Qt::DirectConnection);


    connect(this,&ServerSocketHandle::sig_QueueActionCleanClientList,
            this,&ServerSocketHandle::slot_QueueActionCleanClientList,Qt::QueuedConnection);
    connect(this,&ServerSocketHandle::sig_QueueActionCleanTimerList,
            this,&ServerSocketHandle::slot_QueueActionCleanTimerList,Qt::QueuedConnection);
    connect(this,&ServerSocketHandle::sig_TimerDisable,
            this,&ServerSocketHandle::slot_TimerDisable,Qt::QueuedConnection);
    connect(this,&ServerSocketHandle::sig_QueueSpontaneousClientInfoFlood,
            this,&ServerSocketHandle::slot_QueueSpontaneousClientInfoFlood,Qt::QueuedConnection);
}

bool ServerSocketHandle::GeneralSend(const char *received_packet_total, char *send_packet_total, ClientSession *client,int retry_time) {
    auto* receive_packet_header = (header*)received_packet_total;
    auto* send_packet_header = (header*)send_packet_total;
    uint8_t confirm = 0;
    if(receive_packet_header != nullptr){
        confirm = receive_packet_header->base_proto.timer_id_tied;
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    }
    send_packet_header->base_proto.timer_id_tied = client->server_timer_id = client->server_timer_id%255 +1;
    send_packet_header->base_proto.timer_id_confirm = confirm;
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->base_proto.timer_id_tied,client->socket_fd,send_packet_total,
                                                    kHeaderSize+ ntohl(send_packet_header->base_proto.data_length));
    tmp_timer->set_retry_max(retry_time);
    timer_list_->push_back(tmp_timer);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
        return true;
    }
    else return false;
}



//主循环
void ServerSocketHandle::slot_timeout() {
//    std::cout<<".";
    int ret = select(max_fd_ + 1, server_fd_set_, nullptr, nullptr, &ctl_time_);
    if (ret > 0) {
        if (FD_ISSET(conn_server_fd_, server_fd_set_))//<server>是否存在于ser_fdset集合中,如果存在 意味着有connect请求
        {
            emit sig_EventNewClient();
        } else {
            FD_SET(conn_server_fd_, server_fd_set_);
        }

        for (auto client_iterator = client_list_->begin(); client_iterator != client_list_->end(); ++client_iterator) {
            if (FD_ISSET(client_iterator->socket_fd, server_fd_set_)) {
                clock_gettime(CLOCK_MONOTONIC,&(client_iterator->tick));
                emit sig_MainEventClientPacket(&*client_iterator);
            } else {
                FD_SET(client_iterator->socket_fd, server_fd_set_);
                if (max_fd_ < client_iterator->socket_fd) {
                    max_fd_ = client_iterator->socket_fd;
                }
            }
        }
    }
    else if (ret == 0) {
        FD_SET(conn_server_fd_, server_fd_set_);
        for(auto client_iterator:*client_list_){
            FD_SET(client_iterator.socket_fd,server_fd_set_);
        }
        //region #超时重传相关#
        for(auto &timer_iter:*timer_list_){
            if(timer_iter->TimeoutJustice()){
                timer_iter->TimerTrigger();
                timer_iter->TimerUpdate();
                fflush(stdout);
            }
        }
        emit sig_QueueActionCleanTimerList();
//        endregion
    }
    else {
        perror("select failure\n");
    }
        emit sig_mission_finish(true);
}

//通用的结束方法
void ServerSocketHandle::slot_ActionGeneralFinishSend(const char * receive_packet_total, ClientSession * client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralFinish,
            .detail_code =kNull,
            .timer_id_tied = client->server_timer_id = client->server_timer_id % 255 + 1,
            .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
            .data_length=htonl(0),
    }};
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header.base_proto.timer_id_tied,client->socket_fd,
                                                    (const char *)&send_packet_header,kHeaderSize+ntohl(send_packet_header.chap_proto.data_length));
    tmp_timer->set_retry_max(3);
    timer_list_->push_back(tmp_timer);
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(client->socket_fd,&send_packet_header,kHeaderSize+ntohl(send_packet_header.base_proto.data_length),0);
    }
}

void ServerSocketHandle::slot_ActionGeneralFinishReceive(const char *receive_packet_total, ClientSession * client) {
    emit sig_ActionGeneralAckSend(receive_packet_total,client);
}

void ServerSocketHandle::slot_ActionGeneralAckSend(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralAck,
            .detail_code = receive_packet_header->base_proto.detail_code,
            .timer_id_tied = 0,
            .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
            .data_length = htonl(0)
    }};
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    if(!ErrorSimulator(kGenericAckProb)){
        send(client->socket_fd, &send_packet_header, kHeaderSize + ntohl(send_packet_header.chap_proto.data_length), 0);
    }
}

void ServerSocketHandle::slot_ActionGeneralAckReceive(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
}

//与主对话框有关
void ServerSocketHandle::slot_FlushClientList() {
    emit sig_ClientListClear();
    for(auto &iter:*client_list_){
        if(iter.state == kOnline){
            struct sockaddr_in sa;
            socklen_t len = sizeof(sa);
            if(!getpeername(iter.socket_fd, (struct sockaddr *)&sa, &len)){
                emit sig_ClientListAppend(iter.nickname,inet_ntoa(sa.sin_addr),QString::number(ntohs(sa.sin_port)),iter.now_path);
            }
            else{
                emit sig_ClientListAppend(iter.nickname,"暂不知晓","暂不知晓","暂不知晓");
            }
        }
    }
    emit sig_QueueSpontaneousClientInfoFlood();
}

void ServerSocketHandle::slot_getInfoFromDialog(const QString &ip,
                                                uint16_t port) {
    ip_ = ip;
    port_ = port;
    std::cout<<ip.toStdString()<<":"<<port<<std::endl;
}

void ServerSocketHandle::slot_EventServerLaunch() {
    if(port_ < 0){
        printf("未获得开放端口，请设置后再尝试");
        return;
    }
    struct sockaddr_in server_address{};
    server_address.sin_family= AF_INET;    //IPV4
    server_address.sin_port = htons(port_);
//    server_address.sin_addr.s_addr = INADDR_ANY;  //指定所有地址
    inet_aton(ip_.toLatin1().data(),(in_addr*)&server_address);
    if((conn_server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("创建服务器套接字失败");
        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return;
    }
    //bind socket  绑定
    if(::bind(conn_server_fd_, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("服务端绑定失败");
        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return ;
    }
    if(listen(conn_server_fd_, kBacklog) < 0)
    {
        perror("listen failure");
        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return ;
    }
    FD_SET(conn_server_fd_, server_fd_set_);
    if(max_fd_ < conn_server_fd_)
    {
        max_fd_ = conn_server_fd_;
    }
    printf("Waiting for connection!\n");
    emit hint("Waiting for connection!");
}

//获得存储的数据
void ServerSocketHandle::slot_SaveStorageEnv() {
    char buffer[1024]={0};
    char user_sheet_path[kDirLength + 20];
    strcpy(user_sheet_path,server_data_dir);
    strcat(user_sheet_path,"user_sheet.csv");
    int user_sheet_fd = open(user_sheet_path,O_RDWR|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR);
    if(user_sheet_fd<0){
        perror("open failed");
    }
    for(auto & user_iter : *registered_users_list_){
        sprintf(buffer, "%s,%d\n", user_iter.user_name, user_iter.password);
        write(user_sheet_fd, buffer, strlen(buffer));
        printf("write-> %s", buffer);
    }
    close(user_sheet_fd);
}

void ServerSocketHandle::slot_CreateStorageEnv() {
    mkdir(server_data_dir,S_IRWXU);
    char user_sheet_path[kDirLength + 20];
    char user_root_path[kUserPathMaxLength];
    strcpy(user_sheet_path,server_data_dir);
    strcat(user_sheet_path,"user_sheet.csv");
    if(access(user_sheet_path,R_OK|W_OK|F_OK)==0){
        std::ifstream user_sheet_istream(user_sheet_path);
        string line;
        UserInfo tmp_user{};
        while(getline(user_sheet_istream,line)){
            cout<<"data<< "<<line<<endl;
            string::iterator chr;
            int num_start=0,num_end=0;
            for(chr=line.begin();chr!=line.end();++chr){
                num_end++;
                if(*chr == ','){
                    *chr=0;
                    num_start=num_end;
                }
            }
            string num_str = line.substr(num_start,num_end);
            tmp_user.password=stoi(num_str);
            if(strlen(line.c_str()) > kUsernameLength){
                printf("name:'%s' is too long,drop it",line.c_str());
            }
            strcpy(tmp_user.user_name,line.c_str());
            strcpy(user_root_path,server_data_dir);
            strcat(user_root_path,tmp_user.user_name);

            if(access(user_root_path,R_OK|W_OK|F_OK)<0){
                if(mkdir(user_root_path,S_IRWXU) == 0){
                    cout<<"为用户"<<tmp_user.user_name<<"于"<<user_root_path<<"创建了文件夹"<<endl;
                    strcpy(tmp_user.root_path,user_root_path);
                }
                else{
                    cout<<"试图为用户"<<tmp_user.user_name<<"于"<<user_root_path<<"创建文件夹失败"<<endl;
                    strcpy(tmp_user.root_path,"");
                }
            }else{
                strcpy(tmp_user.root_path,user_root_path);
            }
            registered_users_list_->push_back(tmp_user);
        }


    }
    else{
        emit hint("加载表单失败，使用默认值\n");
        printf("Warning: using default user_list...");
        UserInfo tmp_ui = {.password=12345};
        strcpy(tmp_ui.user_name,"default");
        registered_users_list_->push_back(tmp_ui);
        strcpy(user_root_path,server_data_dir);
        strcat(user_root_path,"default");
        mkdir(user_root_path,S_IRWXU);
    }
}

//计时器控制
void ServerSocketHandle::slot_TimerDisable(uint8_t timer_id, SocketFileDescriptor socket_fd) {
    if(timer_id==0){
        return ;//特判 因为经常用
    }
    for(auto &iter :*timer_list_){
        if(iter->timer_id_==timer_id){
            if(iter->socket_fd_ == socket_fd){
                iter->TimerDisable();
                return;
            }
        }
    }
}

//处理收到的客户端发来的包以及建立新的客户端链接
void ServerSocketHandle::slot_MainEventClientPacket(ClientSession *client) {
    char receive_message[kHeaderSize + kBufferSize]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(client->socket_fd, receive_message, kHeaderSize);
    byte_num += read(client->socket_fd, receive_message + kHeaderSize, ntohl(packet_header->base_proto.data_length));
    if(byte_num >0){
        auto* receive_packet_header = (header*) receive_message;
        switch (receive_packet_header->proto) {
            case kProtoControl:
                switch (receive_packet_header->ctl_proto.ctl_code) {
                    case kControlLogin:
                        emit sig_ActionControlLogin(receive_message,client);
                        break;
                    case kControlLogout:
                        emit sig_ActionControlLogout(receive_message,client);
                        break;
                    case kControlRegister:
                        emit sig_ActionControlRegister(receive_message,client);
                        break;
                    case kControlChangeUsername:
                        break;
                    case kControlChangePassword:
                        break;
                    case kControlLs:
                        emit sig_ActionControlServerFileLsReceive(receive_message,client);
                        break;
                    default:
//                        std::cout<<"receive invalid CTL_code"<<receive_packet_header->ctl_proto.ctl_code;
                        printf("receive invalid CTL_code[code %d]...\n",receive_packet_header->ctl_proto.ctl_code);
                        break;
                }
                return;
            case kProtoMessage:
                emit sig_ActionMessageProcessing(receive_message, client);
                return;
            case kProtoChap:
                switch (receive_packet_header->chap_proto.chap_code) {
                    case kChapResponse:
                        emit sig_ActionChapJustice(receive_message, client);
                        break;
                    default:
                        printf("\tInvalid CHAP_CODE\n");
                }
                return ;
            case kProtoFile:
                switch (packet_header->file_proto.file_code) {
                    case kFileUpload:
                        emit sig_ActionFileUploadReceive(receive_message,client);
                        return;
                    case kFileDownload:
                        emit sig_ActionFileDownloadReceive(receive_message,client);
                        return;
//                    case kFileRequest:
//                        emit sig_ActionFileDownloadReceive(receive_message, client);
//                        return;
//                    case kFileResponse:
//                        emit sig_ActionFileResponseReceive(receive_message,client);
//                        return;
                    case kFileTransporting:
                        emit sig_ActionFileTranslatingReceive(receive_message,client);
                        return ;
                    case kFileAck:
                        emit sig_ActionFileAckReceived(receive_message,client);
                        return;
                    case kFileEnd:
                        emit sig_ActionFileEndReceived(receive_message,client);
                        return;
                    case kFileError:
                        emit sig_ActionFileErrorReceived(receive_message,client);
                        return;
                }
                printf("[sur-rcv] invalid File proto [code %d]...\n",packet_header->file_proto.file_code);
//                cout<<("[sur-rcv] invalid File proto [code]...\n");
                return;
            case kProtoGeneralAck:
                emit sig_ActionGeneralAckReceive(receive_message,client);
                return;
            case kProtoGeneralFinish:
                emit sig_ActionGeneralFinishReceive(receive_message,client);
                return;
        }

    }
    else if(byte_num < 0){
        printf("recv error!");
    } else{//byte_num = 0
        printf("client(%s) exit!\n",client->nickname);
        emit hint("有一个客户端退出了");
        FD_CLR(client->socket_fd, server_fd_set_);
        close(client->socket_fd);
        client->state = kShutdown;
        emit sig_QueueActionCleanClientList();
        return ;//-1
    }
}

void ServerSocketHandle::slot_EventNewClient() {
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(conn_server_fd_, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        ClientSession tmp_cs = {.socket_fd = client_sock_fd,
                .state = kOffline};
        clock_gettime(CLOCK_MONOTONIC,&tmp_cs.tick);
        strcpy(tmp_cs.nickname,"");
        client_list_->push_back(tmp_cs);
        hint("有新的链接产生了...");
        printf("client-kOffline joined in socket_fd(%d)!\n",client_sock_fd);
        fflush(stdout);
    }
    else{
        hint("接受链接出现了问题");
    }
    return;

}

//信息处理
void ServerSocketHandle::slot_ActionMessageProcessing(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total + kHeaderSize);
    printf("message form client(%s):%s\n", client->nickname, receive_packet_data->msg_general.msg_data );
//    ActionGeneralFinishSend(receive_packet_total,client);//msg收到即是结束
    //转发
    char send_packet_total[kHeaderSize+kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    send_packet_header->proto = kProtoMessage;
    strcpy(send_packet_data->msg_general.userName, client->nickname);
    strcpy(send_packet_data->msg_general.msg_data,receive_packet_data->msg_general.msg_data);
    send_packet_header->msg_proto.data_length = htonl(sizeof(UserNameString) + strlen(send_packet_data->msg_general.msg_data));
    bool flood_flag = (strcmp(receive_packet_data->msg_general.userName, "") == 0);
    emit sig_ActionGeneralFinishSend(receive_packet_total,client);
    // 广播到所有客户端或转发到指定的客户端
    for(auto & client_point : *client_list_){
        if(flood_flag){
            if(client_point.socket_fd != client->socket_fd){
//                printf("client->socket_fd=%d", client->socket_fd);
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point.nickname, client_point.socket_fd, send_packet_data->msg_general.msg_data);
                GeneralSend(receive_packet_total,send_packet_total,&client_point);
            }
            else if(client_list_->size() == 1){
                printf("echo message to client(%s).socket_fd=%d with %s\n",
                       client_point.nickname, client_point.socket_fd, send_packet_data->msg_general.msg_data);
                GeneralSend(receive_packet_total,send_packet_total,client);
//                }
            }
        }
        else{
            if(strcmp(client_point.nickname, receive_packet_data->msg_general.userName) == 0){
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point.nickname, client_point.socket_fd, send_packet_data->msg_general.msg_data);
                GeneralSend(receive_packet_total,send_packet_total,&client_point);
            }
        }
    }
}

//登录相关
void ServerSocketHandle::slot_ActionControlLogin(const char *receive_packet_total, ClientSession *client) {
    auto * packet_data  = (data *)(receive_packet_total + kHeaderSize);
    for(auto & logined_user_iter : *client_list_){
        if(!strcmp(logined_user_iter.nickname,packet_data->ctl_login.userName)){
            printf("用户：%s重复登录\n",client->nickname);
            emit sig_ActionControlDuplicateLogin(receive_packet_total,client);
            client->state = kHalfLink;
            return;
        }
    }
    for(auto & user_iter : *registered_users_list_){
        if(!strcmp(user_iter.user_name,packet_data->ctl_login.userName)){
            strcpy(client->nickname, packet_data->ctl_login.userName);
            strcpy(client->now_path, user_iter.root_path);
            printf("用户：%s请求登录\n",client->nickname);
            emit sig_ActionChapChallenge(receive_packet_total,client);
            client->state = kChapChallenging;
            return;
        }
    }
    cout<<"没找到用户名：("<<packet_data->ctl_login.userName<<")"<<endl;
    emit sig_ActionControlUnregistered(receive_packet_total,client);
    client->state = kHalfLink;
}

void ServerSocketHandle::slot_ActionControlUnregistered(const char *receive_packet_total, ClientSession *client) {
    char send_packet[kHeaderSize + kBufferSize];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code=kControlUnregistered;
    send_packet_header->ctl_proto.data_length = htonl(0);
    GeneralSend(receive_packet_total,send_packet,client);
}

void ServerSocketHandle::slot_ActionControlDuplicateLogin(const char *receive_packet_total, ClientSession *client) {
    char send_packet[kHeaderSize + kBufferSize];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code=kControlDuplicateLogin;
    send_packet_header->ctl_proto.data_length = htonl(0);
    GeneralSend(receive_packet_total,send_packet,client);
}

//注册相关
void ServerSocketHandle::slot_ActionControlRegister(const char *receive_packet_total, ClientSession *client) {
    auto * packet_data  = (data *)(receive_packet_total + kHeaderSize);
    for(auto & user_iter : *registered_users_list_){
        if(!strcmp(user_iter.user_name,packet_data->ctl_login.userName)){
            strcpy(client->nickname, packet_data->ctl_login.userName);
            strcpy(client->now_path, user_iter.root_path);
            printf("用户：%s已经注册\n",client->nickname);
            emit sig_ActionControlDuplicateRegister(receive_packet_total, client);
            client->state = kHalfLink;
            return;
        }
    }
    cout<<"没找到用户名：("<<packet_data->ctl_login.userName<<"),可以进行注册"<<endl;
    emit sig_ActionChapChallenge(receive_packet_total,client);
    client->state = kHalfLink;
}

void ServerSocketHandle::slot_ActionControlRegisterSuccess(const char *receive_packet_total, ClientSession *client) {
    char send_packet[kHeaderSize + kBufferSize];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code=kControlRegisterSuccess;
    send_packet_header->ctl_proto.data_length = htonl(0);
    GeneralSend(receive_packet_total,send_packet,client);
    QString tmp = client->nickname;
    emit hint("用户"+tmp+"注册成功");
}

void ServerSocketHandle::slot_ActionControlDuplicateRegister(const char *receive_packet_total, ClientSession *client) {
    char send_packet[kHeaderSize + kBufferSize];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code=kControlDuplicateRegister;
    send_packet_header->ctl_proto.data_length = htonl(0);
    GeneralSend(receive_packet_total,send_packet,client);
}

//注销相关
void ServerSocketHandle::slot_ActionControlLogout(const char *receive_packet_total, ClientSession *client) {
    client->state = kHalfLink;
    emit sig_FlushClientList();
    emit sig_ActionControlLogoutAck(receive_packet_total, client);
}

void ServerSocketHandle::slot_ActionLogoutAck(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize+kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    send_packet_header->base_proto.proto = kProtoControl;
    send_packet_header->base_proto.detail_code = kControlLogout;
    send_packet_header->base_proto.data_length = htonl(0);
    GeneralSend(receive_packet_total,send_packet_total,client);
    //region 范式
//    send_packet_header->ctl_proto.timer_id_tied=client->server_timer_id  = client->server_timer_id % 255 + 1;
//    send_packet_header->ctl_proto.timer_id_confirm = receive_packet_header->base_proto.timer_id_tied;
//    auto* tmp_timer=new TimerRetranslationSession(kGenericTimeInterval, send_packet_header->ctl_proto.timer_id_tied, client->socket_fd, send_packet_total,
//                                                  kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length));
//    timer_list_->push_back(tmp_timer);
//    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
//    if(!ErrorSimulator(kGenericErrorProb)){
//        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
//    }
    //endregion

}

//在线分发用户表单
void ServerSocketHandle::slot_ActionControlClientInfoSend(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize+kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total+kHeaderSize);
    if(kUsernameLength>=sizeof(send_packet_data->ctl_client_info.chr)){
        emit hint("设定的最大姓名长度过长，拒绝分发客户端信息服务");
        return;
    }
    send_packet_header->base_proto.proto = kProtoControl;
    send_packet_header->base_proto.detail_code = kControlClientInfo;
    send_packet_header->base_proto.data_length = htonl(0);
    uint32_t * number =&send_packet_data->ctl_client_info.client_number;
    uint8_t *append = &send_packet_data->ctl_client_info.client_append;
    char * chr_p = send_packet_data->ctl_client_info.chr;
    *number = 0;
    *append = 0;
    for(auto &client_iter:*client_list_){
        if(client_iter.state == kOnline && strlen(client_iter.nickname)<=kUsernameLength){
            if(chr_p - send_packet_data->ctl_client_info.chr + strlen(client_iter.nickname) + 1
               <= sizeof(send_packet_data->ctl_client_info.chr)){//如果剩余长度能塞下这个名字
                strcpy(chr_p,client_iter.nickname);
                chr_p+=strlen(client_iter.nickname)+1;
                *number +=1;
            }
            else{
                send_packet_header->base_proto.data_length = htonl(chr_p - send_packet_data->ctl_client_info.chr
                                                                   + sizeof (send_packet_data->ctl_client_info.client_number)+sizeof (send_packet_data->ctl_client_info.client_append));
                GeneralSend(receive_packet_total,send_packet_total,client);
                *append+=1;
                *number =0;
                chr_p = send_packet_data->ctl_client_info.chr;
                memset(send_packet_data->ctl_client_info.chr,0,sizeof(send_packet_data->ctl_client_info.chr));

                strcpy(chr_p,client_iter.nickname);
                chr_p+=strlen(client_iter.nickname)+1;
                *number +=1;
            }
        }
    }
    send_packet_header->base_proto.data_length = htonl(chr_p - send_packet_data->ctl_client_info.chr
                                                       + sizeof (send_packet_data->ctl_client_info.client_number)+sizeof (send_packet_data->ctl_client_info.client_append));
    GeneralSend(receive_packet_total,send_packet_total,client);


    //region 范式
//    send_packet_header->ctl_proto.timer_id_tied=client->server_timer_id  = client->server_timer_id % 255 + 1;
//    send_packet_header->ctl_proto.timer_id_confirm = receive_packet_header->base_proto.timer_id_tied;
//    auto* tmp_timer=new TimerRetranslationSession(kGenericTimeInterval, send_packet_header->ctl_proto.timer_id_tied, client->socket_fd, send_packet_total,
//                                                  kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length));
//    timer_list_->push_back(tmp_timer);
//    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
//    if(!ErrorSimulator(kGenericErrorProb)){
//        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
//    }
    //endregion
}

//质询相关
void ServerSocketHandle::slot_ActionChapChallenge(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize + kBufferSize]={};
    clock_t seed = clock();
    srand(seed);//这里用time是最经典的 但是因为服务端总是要等很久 每次连接的时间其实差别很大 所以用clock()我觉得不会有问题
    uint8_t  one_data_size = random() % 3; // 2**one_data_size arc4random()是一个非常不错的替代解法 但是这边还是用了rand先
    uint32_t number_count = 10 + random() % (kBufferSize / (1 << one_data_size ) - 10);//至少要有10个数据 （10～40个字节）

    auto sequence = (uint32_t)(random() % ((0U - 1)/4 - chap_list_->size()));
    auto chap_iterator = chap_list_->begin();
    if(!chap_list_->empty()){
        for(chap_iterator = chap_list_->begin();chap_iterator!=chap_list_->end();++chap_iterator){
            if(sequence >= (chap_iterator->sequence)/4){
                sequence+=1;
            } else{
                sequence*=4;
                break;
            }
        }
    } else{
        sequence*=4;
    }

    auto* send_packet_header=(header *)send_packet_total;
    send_packet_header->chap_proto.proto = kProtoChap;
    send_packet_header->chap_proto.chap_code = kChapChallenging;
    send_packet_header->chap_proto.one_data_size=one_data_size;
    send_packet_header->chap_proto.number_count=htonl(number_count);
    send_packet_header->chap_proto.sequence = htonl(sequence);
    union num{
        int32_t int32;
        uint32_t uint32;
        uint16_t uint16;
        uint8_t uint8;
        uint8_t bit[4];
    };
    unsigned int answer=0;
    for(int i = 0; i < number_count ; i++){
        srand(seed+i);
        num tmpnum{.int32=(int32_t)random()};
        tmpnum.uint16 = one_data_size==1 ? htons(tmpnum.uint16):tmpnum.uint16;
        tmpnum.uint32 = one_data_size==2 ? htonl(tmpnum.uint32):tmpnum.uint32;

        memcpy(&(send_packet_total[kHeaderSize + i * (1 << one_data_size)]), &tmpnum, 1 << one_data_size);
        answer += one_data_size==0 ? tmpnum.uint8:0;
        answer += one_data_size==1 ? ntohs(tmpnum.uint16):0;
        answer += one_data_size==2 ? ntohl(tmpnum.uint32):0;
    }
    send_packet_header->chap_proto.data_length = htonl(number_count * (1 << one_data_size));

    send_packet_header->chap_proto.timer_id_tied = client->server_timer_id = client->server_timer_id % 255 + 1;
    send_packet_header->chap_proto.timer_id_confirm = receive_packet_header->chap_proto.timer_id_tied;
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->chap_proto.timer_id_tied,client->socket_fd,
                                                    send_packet_total,kHeaderSize + ntohl(send_packet_header->chap_proto.data_length));
    timer_list_->push_back(tmp_timer);
    emit sig_TimerDisable(receive_packet_header->chap_proto.timer_id_confirm,client->socket_fd);

    ChapSession tmp_chap_session{.sequence=sequence,.answer=answer,.client = client};
    clock_gettime(CLOCK_MONOTONIC,&tmp_chap_session.tick);
    chap_list_->insert(chap_iterator, tmp_chap_session);

//    GeneralSend(receive_packet_total,send_packet_total,client);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->chap_proto.data_length), 0);
    }
}

void ServerSocketHandle::slot_ActionChapJustice(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header *)(receive_packet_total);
    auto* receive_packet_data = (data *)(receive_packet_total + kHeaderSize);
    char send_packet_total[kHeaderSize];
    auto* send_packet_header = (header*)send_packet_total;
    memcpy(send_packet_total, receive_packet_total, kHeaderSize);

    uint32_t sequence = ntohl(receive_packet_header->chap_proto.sequence);
    send_packet_header->chap_proto.sequence=htonl(sequence+1);
    send_packet_header->chap_proto.data_length = htonl(0);

    for (auto chap_iter=chap_list_->begin();chap_iter!= chap_list_->end();++chap_iter) {
        if(chap_iter->sequence == ntohl(receive_packet_header->chap_proto.sequence) - 1){
            for(auto & user_iter : *registered_users_list_){
                if(!strcmp(user_iter.user_name,receive_packet_data->chap_response.userName)&&
                   !strcmp(user_iter.user_name,chap_iter->client->nickname)){
                    if (ntohl(receive_packet_data->chap_response.answer)==(chap_iter->answer^user_iter.password)){
                        send_packet_header->chap_proto.chap_code=kChapSuccess;
                        strcpy(client->nickname,receive_packet_data->chap_response.userName);

                        GeneralSend(receive_packet_total,send_packet_total,client);

                        printf("\tCHAP_SUCCESS,\"%s\" login in socket_fd(%d)\n",client->nickname,client->socket_fd);
                        client->state=kOnline;
                        emit sig_FlushClientList();
                    }
                    else{
                        send_packet_header->chap_proto.chap_code=kChapFailure;
                        GeneralSend(receive_packet_total,send_packet_total,client);

                        printf("\tCHAP_FAILURE in socket_fd(%d)\n",client->socket_fd);
                        client->state=kHalfLink;
                    }
                    printf("\t清除CHAP_challenge记录：sequence:%u answer:%u\n",chap_iter->sequence,chap_iter->answer);
                    chap_list_->erase(chap_iter);
                    return ;
                }
            }

            printf("\tCHAP_ERROR:未在已注册列表中搜寻到CHAP请求的用户名%s,现在尝试注册\n",receive_packet_data->chap_response.userName);
            UserInfo tmp_user{};
            char user_root_path[kUserPathMaxLength];
            strcpy(tmp_user.user_name, receive_packet_data->chap_response.userName);
            tmp_user.password = ntohl(receive_packet_data->chap_response.answer) ^ chap_iter->answer;
            strcpy(user_root_path,server_data_dir);
            strcat(user_root_path,tmp_user.user_name);

            if(access(user_root_path,R_OK|W_OK|F_OK)<0){
                if(mkdir(user_root_path,S_IRWXU) == 0){
                    cout<<"为用户"<<tmp_user.user_name<<"于"<<user_root_path<<"创建了文件夹"<<endl;
                    strcpy(tmp_user.root_path,user_root_path);
                }
                else{
                    cout<<"试图为用户"<<tmp_user.user_name<<"于"<<user_root_path<<"创建文件夹失败"<<endl;
                    strcpy(tmp_user.root_path,"");
                }
            }else{
                strcpy(tmp_user.root_path,user_root_path);
            }
            registered_users_list_->push_back(tmp_user);
            emit sig_ActionControlRegisterSuccess(receive_packet_total,client);
            return ;
        }
    }
    printf("\tCHAP_ERROR:未找到对应的CHAP_challenge记录:username:%s answer:%u\n",
           receive_packet_data->chap_response.userName,ntohl(receive_packet_data->chap_response.answer));
//    std::cout<<"sequence"
//    client->state=kOffline;
}

//队列处理 - 客户端清理，计时器清理，客户端表单泛洪
void ServerSocketHandle::slot_QueueActionCleanTimerList() {
    for(auto timer_iter = timer_list_->begin();timer_iter!=timer_list_->end();){
        if((*timer_iter)->get_timer_state_()==kTimerDisable){
            timer_iter = timer_list_->erase(timer_iter);

//            printf("清除了一个定时器，%d\n",(*timer_iter)->timer_id_);
        }
        else{
            timer_iter++;
        }
    }
}

void ServerSocketHandle::slot_QueueActionCleanClientList() {
    for (auto client_iterator = client_list_->begin(); client_iterator != client_list_->end();) {
        if(client_iterator->state == kShutdown){
            client_iterator = client_list_->erase(client_iterator);
        }
        else{
            ++client_iterator;
        }
    }
    emit sig_FlushClientList();
}

void ServerSocketHandle::slot_QueueSpontaneousClientInfoFlood() {
    for(auto &client_iter:*client_list_){
        if(client_iter.state == kOnline){
            emit sig_ActionControlClientInfoSend(nullptr,&client_iter);
        }
    }
}
//空着好看（不是）
void ServerSocketHandle::slot_ActionFileUploadSend(const char *receive_packet_total, ClientSession *client) {
    // should not do anything
}

void ServerSocketHandle::slot_ActionFileDownloadSend(const char *receive_packet_total, ClientSession *client) {
    // should not do anything
}

//被下载相关 选择重传
void ServerSocketHandle::slot_ActionFileDownloadReceive(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total + kHeaderSize);

    char server_path[kUserPathMaxLength] = {0};
    strcpy(server_path, receive_packet_data->file_download.file_path);

    if(access(server_path,R_OK)==0){
        FileDescriptor  fd_tmp = open(server_path,R_OK);
        if(fd_tmp<0){
            emit sig_ActionFileErrorSend(receive_packet_total,client);
            perror("文件打开失败");
        }
        for(auto iter:*server_file_list_){
            if(iter.session_id == ntohl(receive_packet_header->file_proto.session_id)){
                //是重复的报文
                return;
            }
        }
        FileSession tmp_session{.session_id = ntohl(receive_packet_header->file_proto.session_id),
                .sequence = ntohl(receive_packet_header->file_proto.sequence),
                .file_fd = fd_tmp,
                .state=kFileInit,
                .client = client};
        tmp_session.init_sequence = tmp_session.sequence;
        tmp_session.init_time.tv_sec =ntohll(receive_packet_data->file_upload.init_time.tv_sec);
        tmp_session.init_time.tv_nsec =ntohll(receive_packet_data->file_upload.init_time.tv_nsec);
        server_file_list_->push_back(tmp_session);
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto=kProtoFile;
        send_packet_header->file_proto.file_code = kFileUpload;
        send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_request.init_time));
        send_packet_header->file_proto.session_id =  htonl(tmp_session.session_id);
        send_packet_header->file_proto.sequence = htonl(tmp_session.sequence);

        send_packet_data->file_upload.init_time.tv_sec=htonll(tmp_session.init_time.tv_sec);
        send_packet_data->file_upload.init_time.tv_nsec=htonll(tmp_session.init_time.tv_nsec);
        emit GeneralSend(receive_packet_total,send_packet_total,client);
        printf("收到文件下载请求\n");
        return ;
    }
    emit sig_ActionFileErrorSend(receive_packet_total,client);
}

void ServerSocketHandle::slot_ActionFileTranslatingSend(FileSession * file_session, ClientSession *client) {
//    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    FileSession *file_session_p= file_session;
//    for(auto &file_ss_iter:*server_file_list_){
//        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
//            file_session_p = &file_ss_iter;
//        }
//    }
//    if(file_session_p == nullptr){
//        emit hint("未找到文件传输会话");
//        return;
//    }

    lseek(file_session_p->file_fd, file_session_p->sequence - file_session_p->init_sequence,
          SEEK_SET);//如果在其他操作中临时更改了读写指针的位置，该步骤可以调回来
    send_packet_header->proto = kProtoFile;
    send_packet_header->file_proto.file_code = kFileTransporting;
    ssize_t read_size = read(file_session_p->file_fd,
                             send_packet_data->file_transport.file_data,
                             sizeof(send_packet_data->file_transport.file_data));
    send_packet_header->file_proto.frame_transport_length = htonl(read_size);
    send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_transport.CRC_32)
                                                       + read_size);
    send_packet_header->file_proto.session_id = htonl(file_session_p->session_id);
    send_packet_header->file_proto.sequence = htonl(file_session_p->sequence);//T帧中传送的是发送者自己现在的sequence
    send_packet_data->file_transport.CRC_32 = htonl(0);//这部分就先不做了吧。。
    file_session_p->sequence += read_size;//然后发送者会更新自己的sequence到自己的期望
    file_session_p->state = kFileTransporting;
    if (read_size == 0) {
        emit sig_ActionFileEndSend(send_packet_total, client);//触发关闭
        return;
    } else if (read_size < 0) {
        emit sig_ActionFileErrorSend(send_packet_total, client);//出现异常
        return;
    }
    if (read_size > 0) {
        //此处不能用标准发送，因为不知道具体发了多少。
//            uint8_t confirm = receive_packet_header->base_proto.timer_id_tied;
//            emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, client->socket_fd);

        send_packet_header->base_proto.timer_id_tied = client->server_timer_id = client->server_timer_id % 255 + 1;
        send_packet_header->base_proto.timer_id_confirm = 0;
        auto *tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,
                                                        send_packet_header->base_proto.timer_id_tied, client->socket_fd,
                                                        send_packet_total, kHeaderSize +
                                                                           ntohl(send_packet_header->base_proto.data_length));
        tmp_timer->set_retry_max(10);
        timer_list_->push_back(tmp_timer);
        ssize_t send_size_offset;
        if (!ErrorSimulator(kGenericErrorProb)) {
            send_size_offset = kHeaderSize + ntohl(send_packet_header->base_proto.data_length) -
                               send(client->socket_fd, send_packet_total,
                                    kHeaderSize + ntohl(send_packet_header->base_proto.data_length), 0);
            if (send_size_offset > 0) {
                printf("[*]服务端出现了一次尽力传输\n");
                file_session_p->delay *= 2;//使时延加倍
            } else {
                file_session_p->delay = (file_session_p->delay - 10) > 0 ? (file_session_p->delay - 10) : 1; //使时延略微下降
            }
        }
        file_session_p->sequence -= send_size_offset;
        if(random()%10000<5){
            printf("当前已经传输了 %d KB\n", (file_session_p->sequence - file_session_p->init_sequence) / 1024);
        }
        file_session_p->next_timer->start(file_session_p->delay);
        return;
    }
}

void ServerSocketHandle::slot_ActionFileAckReceived(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*server_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        static int i =0;
        emit hint("未找到文件传输会话:"+QString::number(i++));
        emit sig_ActionFileErrorSend(receive_packet_total,client);
        return;
    }

    if(file_session_p->state == kFileInit){
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, client->socket_fd);

        file_session_p->next_timer = new QTimer(this);
        file_session_p->next_timer->setSingleShot(true);
        connect(file_session_p->next_timer,&QTimer::timeout,[=](){
            emit sig_ActionFileTranslatingSend(file_session_p,client);
        });//需要将以收到报文为核心的思路进行转变
        file_session_p->delay = 50;
        file_session_p->next_timer->start(file_session_p->delay);
    }else if(file_session_p->state == kFileTransporting){
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, client->socket_fd);
    }
    else if(file_session_p->state == kFileEnd){
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, client->socket_fd);
        //上个条件说明发完了 下个条件说明收完了
        if(file_session_p->sequence == ntohl(receive_packet_header->file_proto.sequence)){
            for(auto file_ss_iter = server_file_list_->begin(); file_ss_iter!=server_file_list_->end();++file_ss_iter){
                    close(file_ss_iter->file_fd);
                    server_file_list_->erase(file_ss_iter);
                    break;
            }
        }
    }
    else{
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, client->socket_fd);
    }
//    emit sig_ActionFileTranslatingSend(receive_packet_total,client);
}

void ServerSocketHandle::slot_ActionFileEndSend(const char *receive_packet_total, ClientSession *client) {
    printf("结束本次文件传输\n");
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*server_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)
            && file_ss_iter.client == client){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到待关闭的文件传输会话");
        return;
    }
    if(file_session_p->session_id == ntohl(receive_packet_header->file_proto.session_id)){
        file_session_p->state = kFileEnd;
    }

    send_packet_header->proto = kProtoFile;
    send_packet_header->file_proto.file_code = kFileEnd;
    clock_gettime(CLOCK_MONOTONIC,&send_packet_data->file_end.end_time);
    send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_end));
    send_packet_header->file_proto.session_id = receive_packet_header->file_proto.session_id;
    GeneralSend(receive_packet_total,send_packet_total,client);
}

//被上传相关 停等协议
void ServerSocketHandle::slot_ActionFileUploadReceive(const char *receive_packet_total, ClientSession *client) {
//    char receive_packet_total[kHeaderSize + kBufferSize]={0};
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total + kHeaderSize);
    char client_path[kUserPathMaxLength] = {0};
    char server_path[kUserPathMaxLength] = {0};
    char server_dir[kUserPathMaxLength] = {0};
    char * filename = client_path;
    strcpy(server_path, receive_packet_data->file_upload.file_path);//根据客户端设计 这里收到的一定是一个文件夹
    strcpy(server_dir,server_path);
    strcpy(client_path,receive_packet_data->file_upload.file_path+strlen(server_path)+1);//根据客户端设计 这里收到的一定是一个文件
/*    if(receive_packet_data->file_upload.file_path[0]=='/'){
        strcpy(server_path, receive_packet_data->file_upload.file_path);
    }
    else{
        strcpy(server_path, client->now_path);
        strcat(server_path, "/");
        strcat(server_path, receive_packet_data->file_upload.file_path);
    }*/
    char * last_slush = client_path;
    for(auto &item:client_path){
        if(item=='/'){
            last_slush = &item;
        }
    }
    filename = last_slush+1;

    //检查对应文件夹是否存在 不存在则一层一层的建立
    if(access(server_dir,W_OK)!=0){
        QStringList dirs = QString(server_path).split("/");
        QStringList root = QString(client->now_path).split("/");
        for(int i = 0;i<root.length();i++){
            dirs.replace(i,root[i]);//区分两者根目录
        }
        for(int i = root.length();i<dirs.length();i++){
            if(access(dirs.mid(0,i+1).join("/").toStdString().data(),W_OK)!=0){
                cout<<dirs.mid(0,i+1).join("/").toStdString().data()<<" --目录不存在，将会被创建"<<endl;
                mkdir(dirs.mid(0,i+1).join("/").toStdString().data(),S_IRWXU);
            }
            printf("在服务端确保了%s路径的存在\n",dirs.mid(0,i+1).join("/").toStdString().data());
        }
        cout<<server_dir<<" --目录不存在，将会被创建"<<endl;
        mkdir(server_dir,S_IRWXU);
    }
    strcat(server_path,"/");
    strcat(server_path,filename);

    if(access(server_dir,W_OK)==0){
        FileDescriptor  fd_tmp = open(server_path,O_WRONLY|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
        if(fd_tmp<0){
            emit sig_ActionFileErrorSend(receive_packet_total,client);
//                ActionGeneralFinishSend(receive_packet_total);
            perror("文件打开失败");
        }
        for(auto iter:*server_file_list_){
            if(iter.session_id == ntohl(receive_packet_header->file_proto.session_id)){
                //是重复的报文
                return;
            }
        }
        FileSession tmp_session{.session_id = ntohl(receive_packet_header->file_proto.session_id),
                .sequence = ntohl(receive_packet_header->file_proto.sequence),
                .file_fd = fd_tmp,
                .state=kFileInit,
                .client = client};
        tmp_session.init_sequence = tmp_session.sequence;
        tmp_session.init_time.tv_sec =ntohll(receive_packet_data->file_upload.init_time.tv_sec);
        tmp_session.init_time.tv_nsec =ntohll(receive_packet_data->file_upload.init_time.tv_nsec);
        server_file_list_->push_back(tmp_session);
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto=kProtoFile;
        send_packet_header->file_proto.file_code = kFileDownload;
        send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_request.init_time));
        send_packet_header->file_proto.session_id =  htonl(tmp_session.session_id);
        send_packet_header->file_proto.sequence = htonl(tmp_session.sequence);

        send_packet_data->file_download.init_time.tv_sec=htonll(tmp_session.init_time.tv_sec);
        send_packet_data->file_download.init_time.tv_nsec=htonll(tmp_session.init_time.tv_nsec);
        emit GeneralSend(receive_packet_total,send_packet_total,client);
        printf("准备接受文件");
        return ;
    }
    emit sig_ActionFileErrorSend(receive_packet_total,client);
}

void ServerSocketHandle::slot_ActionFileTranslatingReceive(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*server_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)
        && file_ss_iter.client == client){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到文件传输会话");
        return;
    }

    if((int64_t)(file_session_p->sequence - ntohl(receive_packet_header->file_proto.sequence)) >= 0){
        lseek(file_session_p->file_fd,ntohl(receive_packet_header->file_proto.sequence) - file_session_p->init_sequence,SEEK_SET);//如果在其他操作中临时更改了读写指针的位置，该步骤可以调回来
        ssize_t write_size = write(file_session_p->file_fd,receive_packet_data->file_transport.file_data,
                             ntohl(receive_packet_header->file_proto.frame_transport_length));

        /*char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto = kProtoFile;
        send_packet_header->file_proto.file_code = kFileAck;
        send_packet_header->file_proto.data_length = htonl(0);
        send_packet_header->file_proto.frame_transport_length= htonl(write_size);//另一重含义 实际确收了多少
        send_packet_header->file_proto.session_id = htonl(file_session_p->session_id);
        send_packet_header->file_proto.sequence = htonl(file_session_p->sequence + receive_packet_header->file_proto.frame_transport_length);//对方期望的 确保帧能被识别出来是谁的确认
        send_packet_data->file_transport.CRC_32 = htonl(0);
        file_session_p->sequence += ntohl(write_size);//自己期望的 通常write_size 和 transport_length应当相等。 当写入时磁盘满了的时候可能会发生write_size小的情况 但一般来说...
        file_session_p->state = kFileTransporting;*/

        if(write_size>0){
            emit sig_ActionFileAckSend(receive_packet_total,write_size ,file_session_p,client);
            return;
        }
        else {
            emit sig_ActionFileErrorSend(receive_packet_total,client);
            return;
        }
    }
    else if((int64_t)(file_session_p->sequence - ntohl(receive_packet_header->file_proto.sequence))>0){
        //1.可能回应的时候有丢包 由于自身是有不确认就重发的机制的 此处可以主动处理
        //2.如果发送方发现自己的发送没有完全成功，可能会从尽力传输的位置再次发送 此时需要更改指针
        //以上两种情况想要判断 则系统需要记录每次收发的数量 开销大 直接全部按照第二种情况处理也可
        //最后发现与上一层的代码相同，保留这里的记录，直接更改上一层的条件
        return;
    }
    else if((int64_t)(file_session_p->sequence - ntohl(receive_packet_header->file_proto.sequence))<0){
        //？无法理解为什么会发生这种接收方比发送方还超前的情况 摆着不管
        return;
    }
}


void ServerSocketHandle::slot_ActionFileAckSend(const char *receive_packet_total,ssize_t offset,FileSession * file_session,ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    send_packet_header->proto = kProtoFile;
    send_packet_header->file_proto.file_code = kFileAck;
    send_packet_header->file_proto.data_length = htonl(0);
    send_packet_header->file_proto.frame_transport_length= htonl(offset);//另一重含义 实际确收了多少
    send_packet_header->file_proto.session_id = htonl(file_session->session_id);
    send_packet_header->file_proto.sequence = htonl(file_session->sequence + ntohl(receive_packet_header->file_proto.frame_transport_length));//对方期望的 确保帧能被识别出来是谁的确认
    send_packet_data->file_transport.CRC_32 = htonl(0);
    file_session->sequence += offset;//自己期望的 通常write_size 和 transport_length应当相等。 当写入时磁盘满了的时候可能会发生write_size小的情况 但一般来说...
    file_session->state = kFileTransporting;
    GeneralSend(receive_packet_total, send_packet_total,client);
}

void ServerSocketHandle::slot_ActionFileEndReceived(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    for(auto file_ss_iter = server_file_list_->begin(); file_ss_iter!=server_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)
        && file_ss_iter->client == client){
            close(file_ss_iter->file_fd);
            server_file_list_->erase(file_ss_iter);
            break;
        }
    }
    printf("文件传输完毕");
    emit sig_ActionGeneralFinishSend(receive_packet_total,client);
}

//差错相关
void ServerSocketHandle::slot_ActionFileErrorSend(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    send_packet_header->proto = kProtoFile;
    send_packet_header->file_proto.file_code = kFileError;
    send_packet_header->file_proto.data_length = send_packet_header->file_proto.frame_transport_length = 0;
    send_packet_header->file_proto.session_id = receive_packet_header->file_proto.session_id; // 只用了他
    send_packet_header->file_proto.sequence = receive_packet_header->file_proto.sequence;//不写也行
    for(auto file_ss_iter = server_file_list_->begin(); file_ss_iter!=server_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)
            && file_ss_iter->client == client){
            close(file_ss_iter->file_fd);
            server_file_list_->erase(file_ss_iter);
            break;
        }
    }
    GeneralSend(receive_packet_total,send_packet_total,client);
}

void ServerSocketHandle::slot_ActionFileErrorReceived(const char *receive_packet_total, ClientSession *client) {
    auto* receive_packet_header = (header*)receive_packet_total;
    for(auto file_ss_iter = server_file_list_->begin(); file_ss_iter!=server_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)
            && file_ss_iter->client == client){
            close(file_ss_iter->file_fd);
            server_file_list_->erase(file_ss_iter);
            break;
        }
    }

    emit sig_ActionGeneralFinishSend(receive_packet_total,client);
}

void ServerSocketHandle::slot_ActionControlServerFileLsReceive(const char *receive_packet_total, ClientSession *client) {
    QString client_data = client->now_path;
    client_data+="\n";
    //以下代码是linux限定的..
    DIR *pDir_root;
    struct dirent* ptr;
    if(!(pDir_root = opendir(client->now_path))){
        cout<<"Folder doesn't Exist!"<<endl;
        return;
    }
    else{
        client_data += step_MenuCacheFlush(pDir_root,1,client->now_path);
    }
    char send_packet_total[kHeaderSize+kBufferSize];
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
//    send_packet_total = (char*) malloc(kHeaderSize+strlen(client_data.toUtf8().data())+1);

    send_packet_header->proto=kProtoControl;
    send_packet_header->ctl_proto.ctl_code = kControlLs;
    strcpy(send_packet_data->ctl_ls.chr,client_data.toUtf8().data());
    send_packet_header->ctl_proto.data_length = htonl(strlen(client_data.toUtf8().data())+1);
    GeneralSend(receive_packet_total,send_packet_total,client);
}

QString ServerSocketHandle::step_MenuCacheFlush(DIR *dir , int deep, const QString &str) {
    struct dirent *ptr;
    QString header = "";
    QString rtn = "";
    for(int tmp_deep = deep;tmp_deep>0;tmp_deep--){
        header+="\t";
    }

    while((ptr = readdir(dir)) != nullptr)
    {
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0 && strcmp(ptr->d_name, ".DS_Store") != 0){
//            printf("%s%s",header.toLatin1().data(), ptr->d_name);
            rtn+=header + ptr->d_name+"\n";
            if(ptr->d_type == DT_DIR){
//                printf(" [is a folder]\n");
                rtn+=step_MenuCacheFlush(opendir((str+"/"+ptr->d_name).toLatin1().data()),deep+1,str+"/"+ptr->d_name);
            }
            else{
//                printf("\n");
            }
        }
    }
    closedir(dir);
    return rtn;
}























//*********************************//

ServerSocketThread::ServerSocketThread(QThread *parent,ServerSocketHandle* Server_socket_handle):
        QThread(parent),timer_(new QTimer()){
    if (Server_socket_handle==nullptr){
        server_socket_handle_ = new ServerSocketHandle;
    }
    else{
        server_socket_handle_ = Server_socket_handle;
    }
    timer_->setSingleShot(true);
}

ServerSocketThread::~ServerSocketThread() {
    delete timer_;
    delete server_socket_handle_;
}

void ServerSocketThread::run() {
    QObject::connect(timer_,&QTimer::timeout,server_socket_handle_,&ServerSocketHandle::slot_timeout);
    QObject::connect(server_socket_handle_,&ServerSocketHandle::sig_mission_finish,
                     this, &ServerSocketThread::slot_turn_end);
    exec();
}

void ServerSocketThread::slot_timer_ready() {
    flag_timer_running_ = true;
    timer_->start(1);
}

void ServerSocketThread::slot_turn_end(bool) {
    if(flag_timer_running_){
        timer_->start(1);
    }
}

void ServerSocketThread::stop_timer() {
    flag_timer_running_= false;
}

//********************************************//

//region TimerSession实现代码
TimerSession::TimerSession(double_t time_interval, uint8_t the_timer_id, SocketFileDescriptor socket_fd)
        :timer_id_(the_timer_id),socket_fd_(socket_fd),retry_count_(0),timer_state_(kTimerInit){
    set_timing_interval(time_interval);
    clock_gettime(CLOCK_MONOTONIC,&init_tick_);
}
void TimerSession::set_timing_interval(double_t time_interval) {
    time_interval_.tv_sec = (long)time_interval;
    time_interval_.tv_nsec = (long)(time_interval*1e9 - (time_interval_.tv_sec)*1e9);
}
double_t TimerSession::get_timing_interval() const{
    return (double_t)time_interval_.tv_sec +(double_t)time_interval_.tv_nsec/1e9;
}
void TimerSession::set_retry_max(int retry_max) {
    retry_max_ = retry_max;
}
bool TimerSession::TimeoutJustice() const {
    timespec tmp_time{};
    clock_gettime(CLOCK_MONOTONIC,&tmp_time);
    tmp_time.tv_sec = tmp_time.tv_nsec>=init_tick_.tv_nsec ? tmp_time.tv_sec-init_tick_.tv_sec : tmp_time.tv_sec-init_tick_.tv_sec-1;
    tmp_time.tv_nsec = tmp_time.tv_nsec>=init_tick_.tv_nsec ? tmp_time.tv_nsec-init_tick_.tv_nsec : 1000000000+tmp_time.tv_sec-init_tick_.tv_sec;

//    time_t tmp = tmp_time.tv_sec - init_tick_.tv_sec;

#if CLIENT_DEBUG_LEVEL >2
    printf("(%d):%ld, ",timer_id_,tmp_time.tv_sec);
#endif
    fflush(stdout);
    if(tmp_time.tv_sec>time_interval_.tv_sec){
        return true;
    } else if(tmp_time.tv_sec==time_interval_.tv_sec){
        if(tmp_time.tv_nsec>init_tick_.tv_nsec){
            return true;
        }
    }
    return false;
}
bool TimerSession::TimerUpdate() {
    retry_count_+=1;
    if(retry_count_<retry_max_){
        time_interval_.tv_sec = time_interval_.tv_nsec<500000000 ? time_interval_.tv_sec*2 : time_interval_.tv_sec*2+1;
        time_interval_.tv_nsec = time_interval_.tv_nsec<500000000 ? time_interval_.tv_nsec*2 : time_interval_.tv_nsec*2-100000000;
//        timing_second_*=2;
        clock_gettime(CLOCK_MONOTONIC,&init_tick_);
        return true;
    } else{
//        printf("过长时间没有回应(%d倍初始计时)，定时器%d自毁...\n",(1<<retry_max_)-1,timer_id_);
        TimerDisable();
        return false;
    }
}
bool TimerSession::TimerTrigger(){
    printf("Error:使用了父类的虚函数\n");
    return false;
}
bool TimerSession::TimerDisable() {
    if(timer_state_ == kTimerWorking){
        timer_state_ = kTimerDisable;
//        printf("定时器%d已关闭\n",timer_id_);
        fflush(stdout);
        return true;
    }
    return false;
}

State TimerSession::get_timer_state_(){
    return timer_state_;
}

TimerRetranslationSession::TimerRetranslationSession(double_t timing_interval, uint8_t timer_id, SocketFileDescriptor client_fd,
                                                     const char *packet_cache, size_t packet_length):
        TimerSession(timing_interval, timer_id, client_fd), client_fd_(client_fd), packet_length_(packet_length){
    memcpy(packet_cache_,packet_cache,packet_length);
    timer_state_ = kTimerWorking;
}
bool TimerRetranslationSession::TimerTrigger() {
    if(timer_state_ == kTimerWorking) {
//        TimerUpdate();
        if(!ErrorSimulator(kTimingErrorProb)){
#if CLIENT_DEBUG_LEVEL>2
            printf("\ndebug:定时器(%d)重传成功\n",timer_id_);
            auto* packet_header = (header*)packet_cache_;
            auto* packet_data = (data*)(packet_cache_+kHeaderSize);
#endif
            ssize_t x = send(client_fd_, packet_cache_, packet_length_, 0);
            return  x > 0;
        }
        else{
#if CLIENT_DEBUG_LEVEL>2
            printf("\ndebug:定时器(%d)重传但是出现了差错了\n",timer_id_);
#endif
            return false;
        }
    }
    else
        return false;
}

//endregion

