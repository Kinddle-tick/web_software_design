//
// Created by Kinddle Lee on 2021/12/27.
//

#include "client_socket_handle.h"
using std::endl;
using std::cout;

bool ErrorSimulator(int prob=0){
    if(random()%100 < prob){
        return true;
    }
    else{
        return false;
    }
}

ClientSocketHandle::ClientSocketHandle(QObject *parent) : QObject(parent) {
    client_fd_set_ = new fd_set;
    self_data_ = new ClientSelfData;
    timer_list_ = new QList<TimerSession*>;
    client_file_list_ = new QList<FileSession>;
    MyQtConnection();
}
ClientSocketHandle::~ClientSocketHandle(){
    if(conn_client_fd_!=-1){
        close(conn_client_fd_);
    }
    delete client_fd_set_;
    delete self_data_;
    delete timer_list_;
    delete client_file_list_;
}

void ClientSocketHandle::MyQtConnection() {
    connect(this,&ClientSocketHandle::sig_EventSendMessageToServer,
            this,&ClientSocketHandle::slot_EventSendMessageToServer,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ClientStorageConfirm,
            this,&ClientSocketHandle::slot_ClientStorageConfirm,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_EventReceiveMessageFromServer,
            this,&ClientSocketHandle::slot_EventReceiveMessageFromServer,Qt::DirectConnection);
    connect(this, &ClientSocketHandle::sig_MainEventServerPacket,
            this, &ClientSocketHandle::slot_MainEventServerPacket, Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_EventServerShutdown,
            this,&ClientSocketHandle::slot_EventServerShutdown,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionChapResponse,
            this,&ClientSocketHandle::slot_ActionChapResponse,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionControlServerFileLsSend,
            this,&ClientSocketHandle::slot_ActionControlServerFileLsSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionControlServerFileLsReceive,
            this,&ClientSocketHandle::slot_ActionControlServerFileLsReceive,Qt::DirectConnection);

    connect(this,&ClientSocketHandle::sig_ActionGeneralAckSend,
            this,&ClientSocketHandle::slot_ActionGeneralAckSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionGeneralAckReceive,
            this,&ClientSocketHandle::slot_ActionGeneralAckReceive,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionGeneralFinishSend,
            this,&ClientSocketHandle::slot_ActionGeneralFinishSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionGeneralFinishReceive,
            this,&ClientSocketHandle::slot_ActionGeneralFinishReceive,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionControlClientInfoReceive,
            this,&ClientSocketHandle::slot_ActionControlClientInfoReceive,Qt::DirectConnection);

    connect(this, &ClientSocketHandle::sig_ActionFileUploadSend,
            this, &ClientSocketHandle::slot_ActionFileUploadSend, Qt::DirectConnection);
    connect(this, &ClientSocketHandle::sig_ActionFileDownloadSend,
            this, &ClientSocketHandle::slot_ActionFileDownloadSend, Qt::DirectConnection);
    connect(this, &ClientSocketHandle::sig_ActionFileUploadReceive,
            this, &ClientSocketHandle::slot_ActionFileUploadReceive, Qt::DirectConnection);
    connect(this, &ClientSocketHandle::sig_ActionFileDownloadReceive,
            this, &ClientSocketHandle::slot_ActionFileDownloadReceive, Qt::DirectConnection);
//    connect(this,&ClientSocketHandle::sig_ActionFileResponseSend,
//            this,&ClientSocketHandle::slot_ActionFileResponseSend,Qt::DirectConnection);
//    connect(this,&ClientSocketHandle::sig_ActionFileResponseReceive,
//            this,&ClientSocketHandle::slot_ActionFileResponseReceive,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileTranslatingSend,
            this,&ClientSocketHandle::slot_ActionFileTranslatingSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileTranslatingReceive,
            this,&ClientSocketHandle::slot_ActionFileTranslatingReceive,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileAckSend,
            this,&ClientSocketHandle::slot_ActionFileAckSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileAckReceived,
            this,&ClientSocketHandle::slot_ActionFileAckReceived,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileEndSend,
            this,&ClientSocketHandle::slot_ActionFileEndSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileEndReceived,
            this,&ClientSocketHandle::slot_ActionFileEndReceived,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileErrorSend,
            this,&ClientSocketHandle::slot_ActionFileErrorSend,Qt::DirectConnection);
    connect(this,&ClientSocketHandle::sig_ActionFileErrorReceived,
            this,&ClientSocketHandle::slot_ActionFileErrorReceived,Qt::DirectConnection);


    connect(this,&ClientSocketHandle::sig_TimerDisable,
            this,&ClientSocketHandle::slot_TimerDisable,Qt::QueuedConnection);
    connect(this,&ClientSocketHandle::sig_QueueActionCleanTimerList,
            this,&ClientSocketHandle::slot_QueueActionCleanTimerList,Qt::QueuedConnection);
}

void ClientSocketHandle::slot_SendMessageToSomebody(const QString &str, const QString &who) {
    emit sig_EventSendMessageToServer(str,who);
}

void ClientSocketHandle::slot_timeout() {
    int ret = select(max_fd_ + 1, client_fd_set_, nullptr, nullptr, &ctl_time_);
    if(ret > 0){
        if(conn_client_fd_ > 0){
            if(FD_ISSET(conn_client_fd_, client_fd_set_))
            {
                /** region ## 检查<conn_client_fd>是否存在于ser_fdset集合中,如果存在 意味着服务器发送了内容 ## */
                emit sig_MainEventServerPacket();
                /** endregion */
            }
            else{
                FD_SET(conn_client_fd_, client_fd_set_);
            }
        }
    }

    else if(ret == 0){
        if(conn_client_fd_ > 0){
            FD_SET(conn_client_fd_,client_fd_set_);
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
//endregion

    }

    else{
        perror("select failure");
    }
        emit sig_mission_finish(true);
}

void ClientSocketHandle::slot_getInfoFromDialog(const QString &username, const QString &password, const QString &ip,
                                                uint16_t port) {
    strcpy(self_data_->confirmUserName,username.toLatin1().data());
    self_data_->password = password.toInt();
//    username_=username;
//    password_=password;
    ip_ = ip;
    port_ = port;
    std::cout<<username.toStdString()<<password.toStdString()<<std::endl;
    std::cout<<ip.toStdString()<<":"<<port<<std::endl;
}

void ClientSocketHandle::slot_EventConnectServer() {
    sockaddr_in server_addr{.sin_family=AF_INET,
                            .sin_port = htons(port_)};
    inet_aton(ip_.toLatin1().data(),(in_addr*)&server_addr);
    if(conn_client_fd_>0){
        close(conn_client_fd_);
        FD_CLR(conn_client_fd_,client_fd_set_);
    }
    if((conn_client_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("创建客户端套接字失败");
        emit hint("[-]创建客户端套接字失败");
        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return;
    }
    if(::connect(conn_client_fd_,(struct sockaddr *)&server_addr, sizeof(struct sockaddr_in))==0){
        FD_SET(conn_client_fd_, client_fd_set_);
        if(max_fd_ < conn_client_fd_)
        {
            max_fd_ = conn_client_fd_;
        }
        printf("连接服务器成功");
        self_data_->state = kHalfLink;
        emit hint("[+]连接服务器成功");
        emit state(kHalfLink);
//        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return;
    }
    else{
        perror("连接服务器失败");
        emit hint("[-]连接服务器失败，请检查服务器是否开启及网络链接是否正常");
        emit sig_ErrorCodeSend(kSocketErrorConnectRefused);
        return;
    }

}

void ClientSocketHandle::slot_EventSendMessageToServer(const QString &message, const QString &username) {
    if(self_data_->state == kOnline) {
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto * send_packet_header=(header*)send_packet_total;
        auto * send_packet_data = (data *)(send_packet_total + kHeaderSize);

        send_packet_header->msg_proto.proto = kProtoMessage;
        ::strcpy(send_packet_data->msg_general.userName, username.toUtf8().data());
        ::strcpy(send_packet_data->msg_general.msg_data, message.toUtf8().data());
        send_packet_header->msg_proto.data_length = htonl(strlen(send_packet_data->msg_general.msg_data)
                                                          + sizeof(send_packet_data->msg_general.userName));

        send_packet_header->msg_proto.timer_id_tied= client_timer_id_= client_timer_id_ % 255 + 1;
        send_packet_header->msg_proto.timer_id_confirm=0;
        auto*tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->msg_proto.timer_id_tied,conn_client_fd_,
                                                       send_packet_total,kHeaderSize+ ntohl(send_packet_header->msg_proto.data_length));
        timer_list_->push_back(tmp_timer);
        emit sig_TimerDisable(0,conn_client_fd_);
//        TimerDisable(0,conn_client_fd_);
        if(!ErrorSimulator(kGenericErrorProb)){
            send(conn_client_fd_, send_packet_total, kHeaderSize + ntohl(send_packet_header->msg_proto.data_length), 0);
        }
        std::cout << "[send->server] message send to server:" << send_packet_data->msg_general.msg_data << std::endl;
//        cue_flag = true;
        return ;
    }
    else{
        cout<<"[send->server] Error: please connect server..."<<endl;
        emit hint("[-]先登录才能发送消息");
        return ;//-1
    }
}

void ClientSocketHandle::slot_EventReceiveMessageFromServer(const char *receive_packet_total) {
    auto * receive_packet_header=(header*)receive_packet_total;
    auto * receive_packet_data = (data *)(receive_packet_total + kHeaderSize);
    QString msg = QString::fromUtf8(receive_packet_data->msg_general.msg_data);
    QString username = QString::fromUtf8(receive_packet_data->msg_general.userName);
//    emit hint(username + " say:" + msg);
    emit message(msg, username, false);
    emit sig_ActionGeneralFinishSend(receive_packet_total);
}

void ClientSocketHandle::slot_TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd) {
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

void ClientSocketHandle::slot_EventServerShutdown() {
    FD_CLR(conn_client_fd_,client_fd_set_);
    close(conn_client_fd_);
    conn_client_fd_ = -1;
    emit hint("[-]服务器断开...");
}

void ClientSocketHandle::slot_EventLogin() {
    if(self_data_->state != kOffline){
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto = kProtoControl;
        send_packet_header->ctl_proto.ctl_code = kControlLogin;
        memcpy(send_packet_data->ctl_login.userName, self_data_->confirmUserName, kUsernameLength);
        send_packet_header->ctl_proto.data_length = htonl(sizeof(send_packet_data->ctl_login));

        GeneralSend(nullptr,send_packet_total);
        self_data_->state = kHalfLink;
        emit state(kHalfLink);
        return ;
    } else{
        cout<< "can not send <login> packet to server in state <kOffline>" <<endl;
        emit hint("当前离线，不能登录");
        return;
    }
}

void ClientSocketHandle::slot_EventLogout() {
    if(self_data_->state == kOnline){
        char send_packet_total[kHeaderSize+kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
//        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto = kProtoControl;
        send_packet_header->ctl_proto.ctl_code = kControlLogout;
//        GeneralSend(nullptr,send_packet_total);
        //region 范式
        send_packet_header->ctl_proto.timer_id_tied= client_timer_id_ = client_timer_id_ % 255 + 1;
        send_packet_header->ctl_proto.timer_id_confirm = 0;//作为流程中第一个触发的报文 不会存在在另一侧绑定的报文数字
        auto* tmp_timer=new TimerRetranslationSession(kGenericTimeInterval, send_packet_header->ctl_proto.timer_id_tied, conn_client_fd_, send_packet_total,
                                                      kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length));
        timer_list_->push_back(tmp_timer);
        emit sig_TimerDisable(0,conn_client_fd_);
        if(!ErrorSimulator(kGenericErrorProb)){
            send(conn_client_fd_, send_packet_total, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
        }
        //endregion

        return;
    }
    else{
        emit hint("只有在线时才能进行注销(登出)操作");
    }
}

void ClientSocketHandle::slot_EventRegister() {
    if(self_data_->state == kHalfLink){
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto = kProtoControl;
        send_packet_header->ctl_proto.ctl_code = kControlRegister;
        memcpy(send_packet_data->ctl_login.userName, self_data_->confirmUserName, kUsernameLength);
        send_packet_header->ctl_proto.data_length = htonl(sizeof(send_packet_data->ctl_login));

        GeneralSend(nullptr,send_packet_total);
        self_data_->state = kHalfLink;
        emit state(kHalfLink);
        return ;
    }
    else{
        cout<< "只有在连接到服务器但尚未登录时才能进行注册" <<endl;
        emit hint("只有在连接到服务器但尚未登录时才能进行注册");
        return;
    }
}

void ClientSocketHandle::slot_ActionChapResponse(const char *receive_packet_total) {
    auto* receive_packet_header=(header*)receive_packet_total;
    int one_data_size = receive_packet_header->chap_proto.one_data_size;
    uint answer = 0;
    union num{
        int32_t int32;
        uint32_t uint32;
        uint16_t uint16;
        uint8_t uint8;
    };
    for(int i = 0;i<ntohl(receive_packet_header->chap_proto.number_count); i++){
        num tmp_num{};
        memcpy(&tmp_num, receive_packet_total + kHeaderSize + i * (1 << one_data_size), 1 << one_data_size);
        answer += one_data_size==0 ? tmp_num.uint8 : 0;
        answer += one_data_size==1 ? ntohs(tmp_num.uint16) : 0;
        answer += one_data_size==2 ? ntohl(tmp_num.uint32) : 0;
    }

    char send_packet_total[kHeaderSize + kBufferSize];
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data=(data*)(send_packet_total + kHeaderSize);
//    memcpy(send_packet_header, receive_packet_header, kHeaderSize);
    send_packet_header->proto = kProtoChap;
    send_packet_header->chap_proto.chap_code=kChapResponse;
    send_packet_header->chap_proto.data_length = htonl(sizeof(send_packet_data->chap_response));
    send_packet_header->chap_proto.number_count = receive_packet_header->chap_proto.number_count;
    send_packet_header->chap_proto.sequence= htonl(ntohl(receive_packet_header->chap_proto.sequence)+1);
    send_packet_header->chap_proto.one_data_size = receive_packet_header->chap_proto.one_data_size;

    memcpy(send_packet_data->chap_response.userName, self_data_->confirmUserName, kUsernameLength);
    send_packet_data->chap_response.answer = htonl(answer^self_data_->password);
//    send_packet_header->chap_proto.data_length = htonl(sizeof(send_packet_data->chap_response));
    GeneralSend(receive_packet_total,send_packet_total);
}

void ClientSocketHandle::slot_ActionGeneralFinishSend(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralFinish,
            .detail_code =kNull,
            .timer_id_tied = client_timer_id_ = client_timer_id_ % 255 + 1,
            .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
            .data_length=htonl(0),
    }};
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header.base_proto.timer_id_tied,conn_client_fd_,
                                                    (const char *)&send_packet_header,kHeaderSize+ntohl(send_packet_header.chap_proto.data_length));
    tmp_timer->set_retry_max(3);
    timer_list_->push_back(tmp_timer);
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd_);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(conn_client_fd_,&send_packet_header,kHeaderSize+ntohl(send_packet_header.base_proto.data_length),0);
    }
}

void ClientSocketHandle::slot_ActionGeneralFinishReceive(const char *receive_packet_total) {
    emit sig_ActionGeneralAckSend(receive_packet_total);
}

void ClientSocketHandle::slot_ActionGeneralAckSend(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralAck,
            .detail_code = receive_packet_header->base_proto.detail_code,
            .timer_id_tied = 0,
            .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
            .data_length = htonl(0)}};
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd_);
    if(!ErrorSimulator(kGenericAckProb)){
        send(conn_client_fd_, &send_packet_header, kHeaderSize + ntohl(send_packet_header.base_proto.data_length), 0);
    }
}

void ClientSocketHandle::slot_ActionGeneralAckReceive(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd_);
}

void ClientSocketHandle::slot_QueueActionCleanTimerList() {
    for(auto timer_iter = timer_list_->begin();timer_iter!=timer_list_->end();){
        if((*timer_iter)->get_timer_state_()==kTimerDisable){
            timer_iter = timer_list_->erase(timer_iter);
        }
        else{
            timer_iter++;
        }
    }
}

bool ClientSocketHandle::GeneralSend(const char *received_packet_total, char *send_packet_total, int retry_time) {
    auto* send_packet_header = (header*)send_packet_total;
    auto* receive_packet_header = (header*)received_packet_total;
    uint8_t confirm = 0;
    if(receive_packet_header != nullptr){
        confirm = receive_packet_header->base_proto.timer_id_tied;
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd_);
    }
    send_packet_header->base_proto.timer_id_tied = client_timer_id_ = client_timer_id_%255 + 1;
    send_packet_header->base_proto.timer_id_confirm = confirm;
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->base_proto.timer_id_tied,conn_client_fd_,
                                                    send_packet_total,kHeaderSize + ntohl(send_packet_header->base_proto.data_length));
    tmp_timer->set_retry_max(retry_time);
    timer_list_->push_back(tmp_timer);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(conn_client_fd_, send_packet_total, kHeaderSize + ntohl(send_packet_header->base_proto.data_length), 0);
        return true;
    }
    else return false;
}

void ClientSocketHandle::slot_ActionControlClientInfoReceive(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char * chr_p = receive_packet_data->ctl_client_info.chr;
    if(receive_packet_data->ctl_client_info.client_append==0){
        emit sig_ClientListClear();
    }
    for(int count =0;count<receive_packet_data->ctl_client_info.client_number;count++){
        emit sig_ClientListAppend(chr_p);
        chr_p += strlen(chr_p)+1;
    }
    emit sig_ActionGeneralAckSend(receive_packet_total);
}

//下载相关函数 选择重传
void ClientSocketHandle::slot_ActionFileDownloadSend(const char* file_path_server, const char* file_path_client) {
    if(self_data_->state == kOnline){
        char server_path[kUserPathMaxLength] = {0};
        char client_path[kUserPathMaxLength] = {0};
        char client_dir[kUserPathMaxLength] = {0};
        char * filename=server_path;
        strcpy(client_path,file_path_client);
        strcpy(server_path,file_path_server);
        strcpy(client_dir,client_path); // 下载时，客户端常常是文件夹 但还是需要检查一下
        char * last_slush = nullptr;
        char * last_point = nullptr;

        //*客户端路径检查
        last_slush = last_point = client_dir;
        for(auto &item:client_dir){
            if(item=='/'){
                last_slush = &item;
            }
            if(item == '.'){
                last_point = &item;
            }
        }
        //客户端路径选择了一个文件
        if(last_point>last_slush){
            *last_slush=0;//client_dir的文件部分将会被去除 效果为下载的文件放在选中文件相同的文件夹中
        }
        else if(last_point==last_slush){
            emit hint("无效的客户端路径");
            return;//开摆
        }
            //客户端选择的是文件夹... 要确保这个文件夹是存在的
        else if(last_point<last_slush){
            QStringList dirs = QString(client_dir).split("/");
            for(int i = 0;i<dirs.length();i++){
                if(access(dirs.mid(0,i+1).join("/").toStdString().data(),W_OK)!=0){
                    cout<<dirs.mid(0,i+1).join("/").toStdString().data()<<" --目录不存在，将会被创建"<<endl;
                    mkdir(dirs.mid(0,i+1).join("/").toStdString().data(),S_IRWXU);
                }
                printf("在客户端确保了%s路径的存在\n",dirs.mid(0,i+1).join("/").toStdString().data());
            }
        }

        //*服务端路径检查
        last_slush = last_point = server_path;
        for(auto &item:server_path){
            if(item=='/'){
                last_slush = &item;
            }
            if(item == '.'){
                last_point = &item;
            }
        }
        //服务端选择的是文件
        if(last_point>last_slush){
            filename = last_slush+1; //定位到文件名
        }
        else if(last_point==last_slush){//服务端什么都不是 没有斜杠也没有点 结合我们目录的特性 这种情况一般就是空白（或者点到了前面）
            emit hint("无效的服务端路径");
            return;//摆烂
        }
            //服务端选择的是文件夹... 创建这个文件夹就可以了 也就不劳烦和服务端互动了
        else if(last_point<last_slush){
            QStringList dirs = QString(server_path).split("/");
            QStringList root = QString(self_data_->client_data_dir_now).split("/");
            for(int i = 0;i<root.length();i++){
                dirs.replace(i,root[i]);//区分两者根目录
            }
            for(int i = root.length();i<dirs.length();i++){
                if(access(dirs.mid(0,i+1).join("/").toStdString().data(),W_OK)!=0){
                    cout<<dirs.mid(0,i+1).join("/").toStdString().data()<<" --目录不存在，将会被创建"<<endl;
                    mkdir(dirs.mid(0,i+1).join("/").toStdString().data(),S_IRWXU);
                }
                printf("在客户端确保了%s路径的存在\n",dirs.mid(0,i+1).join("/").toStdString().data());
            }
            return;
        }

        strcpy(client_path,client_dir);
        strcat(client_path,"/");
        strcat(client_path,filename);
        if(access(client_dir,W_OK)==0){
            FileDescriptor fd_tmp = open(client_path,O_WRONLY|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
            if(fd_tmp<0){
                perror("文件打开失败");
                return;
            }
            FileSession tmp_session{.session_id = (session_id_++),
                    .sequence = (uint32_t)(random()),
                    .file_fd = fd_tmp,
                    .state=kFileInit,};
            tmp_session.init_sequence = tmp_session.sequence;
            clock_gettime(CLOCK_MONOTONIC,&tmp_session.init_time);
            client_file_list_->push_back(tmp_session);
            char send_packet_total[kHeaderSize + kBufferSize]={0};
            auto* send_packet_header = (header*)send_packet_total;
            auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
            send_packet_header->proto=kProtoFile;
            send_packet_header->file_proto.file_code = kFileDownload;
            send_packet_header->file_proto.data_length = htonl(strlen(server_path)+strlen(client_path)+2 + sizeof(send_packet_data->file_request.init_time));
            send_packet_header->file_proto.session_id =  htonl(tmp_session.session_id);
            send_packet_header->file_proto.sequence = htonl(tmp_session.sequence);

            strcpy(send_packet_data->file_download.file_path, server_path);
            strcpy(send_packet_data->file_download.file_path+ strlen(server_path)+1, client_path);
            send_packet_data->file_download.init_time.tv_sec=htonll(tmp_session.init_time.tv_sec);
            send_packet_data->file_download.init_time.tv_nsec=htonll(tmp_session.init_time.tv_nsec);
            emit GeneralSend(nullptr,send_packet_total);
            printf("发出了对文件的下载请求\n");
            return ;
        }
        else{
            cout<<"该路径文件不存在或文件不可用:"<<client_dir<<endl;
            return;
        }
    }
    else{
        cout<< "can only send download cmd to server in state <kOnline>" <<endl;
        return;
    }
}

void ClientSocketHandle::slot_ActionFileUploadReceive(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*client_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到文件传输会话- upload receive");
        return;
    }
    emit sig_ActionFileAckSend(receive_packet_total,0,file_session_p);
}

void ClientSocketHandle::slot_ActionFileTranslatingReceive(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*client_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到文件传输会话-trans receive");
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
            emit sig_ActionFileAckSend(receive_packet_total,write_size ,file_session_p);
            return;
        }
        else {
            emit sig_ActionFileErrorSend(receive_packet_total);
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

void ClientSocketHandle::slot_ActionFileAckSend(const char *receive_packet_total,ssize_t offset,FileSession * file_session) {
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

//    auto* send_packet_header = (header*)send_packet_total;
//    auto* receive_packet_header = (header*)receive_packet_total;
    uint8_t confirm = 0;
    if(receive_packet_header != nullptr){
        confirm = receive_packet_header->base_proto.timer_id_tied;
        emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd_);
    }
    send_packet_header->base_proto.timer_id_tied = 0;
    send_packet_header->base_proto.timer_id_confirm = confirm;

    if(!ErrorSimulator(kGenericErrorProb)){
        send(conn_client_fd_, send_packet_total, kHeaderSize + ntohl(send_packet_header->base_proto.data_length), 0);
    }

}

void ClientSocketHandle::slot_ActionFileEndReceived(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    for(auto file_ss_iter = client_file_list_->begin(); file_ss_iter!=client_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)){
            close(file_ss_iter->file_fd);
            timespec tmp_time{};
            clock_gettime(CLOCK_MONOTONIC,&tmp_time);
            tmp_time.tv_sec = tmp_time.tv_nsec>=file_ss_iter->init_time.tv_nsec
                              ?tmp_time.tv_sec-file_ss_iter->init_time.tv_sec
                              :tmp_time.tv_sec - file_ss_iter->init_time.tv_sec-1;
            tmp_time.tv_nsec = tmp_time.tv_nsec>=file_ss_iter->init_time.tv_nsec
                               ?tmp_time.tv_nsec - file_ss_iter->init_time.tv_nsec
                               :1000000000+tmp_time.tv_nsec - file_ss_iter->init_time.tv_nsec;
            cout<<"一个文件处理结束，用时："<<tmp_time.tv_sec<<"s "<<(double long)tmp_time.tv_nsec/1e6L<<"ms"<<endl;

            client_file_list_->erase(file_ss_iter);
            printf("文件传输完毕\n");
            emit sig_ActionGeneralFinishSend(receive_packet_total);
            return;
        }
    }

    printf("试图关闭文件会话时，没有找到符合条件的成员(可能是重传导致)\n");
}

//上传相关函数 停等协议
void ClientSocketHandle::slot_ActionFileUploadSend(const char* file_path_server, const char* file_path_client) {
    if(self_data_->state == kOnline){
        char server_path[kUserPathMaxLength] = {0};
        char client_path[kUserPathMaxLength] = {0};
        char server_dir[kUserPathMaxLength] = {0};
        char * filename=client_path;
        strcpy(client_path,file_path_client);
        strcpy(server_path,file_path_server);
        /*if(client_path[0]=='/'){
            strcpy(client_path,client_path);
        }
        else{
            strcpy(client_path,self_data_->client_data_dir_now);
            strcat(client_path,"/");
            strcat(client_path,client_path);
        }*/
        // 上传时，客户端常常是文件 但还是需要检查一下
        char * last_slush = nullptr;
        char * last_point = nullptr;
        last_slush = last_point = client_path;
        for(auto &item:client_path){
            if(item=='/'){
                last_slush = &item;
            }
            if(item == '.'){
                last_point = &item;
            }
        }
        //客户端路径选择了一个文件
        if(last_point>last_slush){
            filename = last_slush+1;
        }
            //客户端选择的东西意义不明
        else if(last_point == last_slush){
            return;
        }
            //客户端路径选择的是文件夹
        else if (last_point<last_slush){
            //细想了一下感觉这样不是很合理 如果客户端选择的不是文件而上传，是想将文件夹建立在什么地方呢？？
            //所以如果选择的是文件夹那么也摆烂
            hint("上传时必须选择文件");
            return;
            //不需要准备描述符 直接发过去让服务器建立文件夹即可
//            char send_packet_total[kHeaderSize + kBufferSize]={0};
//            auto* send_packet_header = (header*)send_packet_total;
//            auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
//            send_packet_header->proto=kProtoFile;
//            send_packet_header->file_proto.file_code = kFileUpload;
//            send_packet_header->file_proto.data_length = htonl(strlen(server_path)+strlen(client_path)+2 + sizeof(send_packet_data->file_request.init_time));
//            strcpy(send_packet_data->file_upload.file_path, server_path);
//            strcpy(send_packet_data->file_upload.file_path+ strlen(server_path)+1, client_path);
//            emit GeneralSend(nullptr,send_packet_total);
//            printf("尝试让服务端建立了文件夹\n");
//            return ;
        }

        // 上传时，服务端常常是文件夹
        strcpy(server_dir,server_path);
        last_slush = last_point = server_dir;
        for(auto &item:server_dir){
            if(item=='/'){
                last_slush = &item;
            }
            if(item == '.'){
                last_point = &item;
            }
        }
        //服务端选择的是文件
        if(last_point>last_slush){
            *last_slush = 0;
        }
        else if(last_point==last_slush){//服务端什么都不是 没有斜杠也没有点 结合我们目录的特性 这种情况一般就是空白（或者点到了前面）
            return;//摆烂
        }
            //服务端选择的是文件夹
        else if(last_point<last_slush){
            // 什么也不需要做
        }

        if(access(client_path,R_OK)==0){
            FileDescriptor fd_tmp = open(client_path,O_RDONLY);
            if(fd_tmp<0){
//                emit sig_ActionFileErrorSend()
//                ActionGeneralFinishSend(receive_packet_total);
                perror("文件打开失败");
                return;
            }
            FileSession tmp_session{.session_id = ntohl(session_id_++),
                    .sequence = ntohl(random()),
                    .file_fd = fd_tmp,
                    .state=kFileInit,};
            tmp_session.init_sequence = tmp_session.sequence;
            clock_gettime(CLOCK_MONOTONIC,&tmp_session.init_time);
            client_file_list_->push_back(tmp_session);

            char send_packet_total[kHeaderSize + kBufferSize]={0};
            auto* send_packet_header = (header*)send_packet_total;
            auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
            send_packet_header->proto=kProtoFile;
            send_packet_header->file_proto.file_code = kFileUpload;
            send_packet_header->file_proto.data_length = htonl(strlen(server_path)+strlen(client_path)+2 + sizeof(send_packet_data->file_request.init_time));
            send_packet_header->file_proto.session_id =  htonl(tmp_session.session_id);
            send_packet_header->file_proto.sequence = htonl(tmp_session.sequence);

            strcpy(send_packet_data->file_upload.file_path, server_path);
            strcpy(send_packet_data->file_upload.file_path+ strlen(server_path)+1, client_path);
            send_packet_data->file_upload.init_time.tv_sec=htonll(tmp_session.init_time.tv_sec);
            send_packet_data->file_upload.init_time.tv_nsec=htonll(tmp_session.init_time.tv_nsec);
            emit GeneralSend(nullptr,send_packet_total);
            return ;
        }
        else{
            cout<<"该路径文件不存在或文件不可用:"<<client_path<<endl;
            return;
        }
    }
    else{
        cout<< "can only send upload cmd to server in state <kOnline>" <<endl;
        return;
    }
}

void ClientSocketHandle::slot_ActionFileDownloadReceive(const char *receive_packet_total) {
    emit sig_ActionFileTranslatingSend(receive_packet_total);
}

void ClientSocketHandle::slot_ActionFileTranslatingSend(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*client_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到文件传输会话-trans send");
        return;
    }
    //确认帧的序列号和本地存储的期望序列号相等
    if((int64_t)(ntohl(receive_packet_header->file_proto.sequence)-file_session_p->sequence) >=0){
        lseek(file_session_p->file_fd,file_session_p->sequence - file_session_p->init_sequence,SEEK_SET);//如果在其他操作中临时更改了读写指针的位置，该步骤可以调回来
        send_packet_header->proto = kProtoFile;
        send_packet_header->file_proto.file_code = kFileTransporting;
        ssize_t read_size = read(file_session_p->file_fd,
                                 send_packet_data->file_transport.file_data,
                                 sizeof(send_packet_data->file_transport.file_data));
        send_packet_header->file_proto.frame_transport_length= htonl(read_size);
        send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_transport.CRC_32)
                                                           +read_size);

        send_packet_header->file_proto.session_id = htonl(file_session_p->session_id);
        send_packet_header->file_proto.sequence = htonl(file_session_p->sequence);//T帧中传送的是发送者自己现在的sequence
        send_packet_data->file_transport.CRC_32 = htonl(0); //这部分就先不做了吧。。
        file_session_p->sequence += read_size;//然后发送者会更新自己的sequence到自己的期望
        file_session_p->state = kFileTransporting;
        if(read_size==0){
            emit sig_ActionFileEndSend(receive_packet_total);//触发关闭
            return;
        }
        else if(read_size<0){
            emit sig_ActionFileErrorSend(receive_packet_total);//出现异常
            return;
        }
        if(read_size>0){
            //此处不能用标准发送，因为不知道具体发了多少。
            uint8_t confirm = receive_packet_header->base_proto.timer_id_tied;
            emit sig_TimerDisable(receive_packet_header->base_proto.timer_id_confirm, conn_client_fd_);

            send_packet_header->base_proto.timer_id_tied = client_timer_id_ = client_timer_id_%255 + 1;
            send_packet_header->base_proto.timer_id_confirm = confirm;
            auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->base_proto.timer_id_tied,conn_client_fd_,
                                                            send_packet_total,kHeaderSize + ntohl(send_packet_header->base_proto.data_length));
            tmp_timer->set_retry_max(10);
            timer_list_->push_back(tmp_timer);
            ssize_t send_size_offset;
            if(!ErrorSimulator(kGenericErrorProb)){
                send_size_offset = kHeaderSize + ntohl(send_packet_header->base_proto.data_length) - send(conn_client_fd_, send_packet_total, kHeaderSize + ntohl(send_packet_header->base_proto.data_length), 0);
                if(send_size_offset > 0){
                    printf("[*]客户端出现了一次尽力传输\n");
                }
            }
            file_session_p->sequence -= send_size_offset;
            printf("当前已经传输了 %d KB\n",(file_session_p->sequence - file_session_p->init_sequence)/1024);

//            GeneralSend(receive_packet_total,send_packet_total);
            return;
        }
    }
    else if((int64_t)(ntohl(receive_packet_header->file_proto.sequence)-file_session_p->sequence) <0){
        //可能有丢包 由于自身是有不确认就重发的机制的 此处可以主动处理
        return;
    }
    else if((int64_t)(ntohl(receive_packet_header->file_proto.sequence)-file_session_p->sequence) >0){
        //当发送方在上一轮发送中意识到有数据没有正常的传输出去以后，会将自身的序列期望降低到已经正常发送的数据位置
        //接收方无法知晓数据是否全部传输到位，所以会返回一个偏大的确认
        //此时发送方只需要从之前已经尽力发送的位置继续发送即可
        //这种情况的代码和相等的是一样的，分析保留 但第一分支的条件略作改动
        return;
    }

}

void ClientSocketHandle::slot_ActionFileAckReceived(const char *receive_packet_total) {
    emit sig_ActionFileTranslatingSend(receive_packet_total);
}

void ClientSocketHandle::slot_ActionFileEndSend(const char *receive_packet_total) {
    printf("结束本次文件传输\n");
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    FileSession *file_session_p= nullptr;
    for(auto &file_ss_iter:*client_file_list_){
        if (file_ss_iter.session_id==ntohl(receive_packet_header->file_proto.session_id)){
            file_session_p = &file_ss_iter;
        }
    }
    if(file_session_p == nullptr){
        emit hint("未找到待关闭的文件传输会话");
        return;
    }
    if(file_session_p->session_id == ntohl(receive_packet_header->file_proto.session_id)){
        close(file_session_p->file_fd);
        timespec tmp_time{};
        clock_gettime(CLOCK_MONOTONIC,&tmp_time);
        tmp_time.tv_sec = tmp_time.tv_nsec>=file_session_p->init_time.tv_nsec
                          ?tmp_time.tv_sec-file_session_p->init_time.tv_sec
                          :tmp_time.tv_sec - file_session_p->init_time.tv_sec-1;
        tmp_time.tv_nsec = tmp_time.tv_nsec>=file_session_p->init_time.tv_nsec
                           ?tmp_time.tv_nsec - file_session_p->init_time.tv_nsec
                           :1000000000+tmp_time.tv_nsec - file_session_p->init_time.tv_nsec;
        cout<<"一个文件处理结束，用时："<<tmp_time.tv_sec<<"s "<<(double long)tmp_time.tv_nsec/1e6L<<"ms"<<endl;
        for(auto file_ss_iter = client_file_list_->begin(); file_ss_iter!=client_file_list_->end();++file_ss_iter){
            if(file_ss_iter->session_id == ntohl(receive_packet_header->file_proto.session_id)) {
                close(file_ss_iter->file_fd);
                client_file_list_->erase(file_ss_iter);
                break;
            }
        }
    }

    send_packet_header->proto = kProtoFile;
    send_packet_header->file_proto.file_code = kFileEnd;
    send_packet_header->file_proto.session_id =receive_packet_header->file_proto.session_id;

    clock_gettime(CLOCK_MONOTONIC,&send_packet_data->file_end.end_time);
    send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_end));
    GeneralSend(receive_packet_total,send_packet_total);
}

//差错相关 不区分 除了重大的错误一般不触发
void ClientSocketHandle::slot_ActionFileErrorSend(const char *receive_packet_total) {
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
    for(auto file_ss_iter = client_file_list_->begin(); file_ss_iter!=client_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)){
            close(file_ss_iter->file_fd);
            client_file_list_->erase(file_ss_iter);
            break;
        }
    }
    printf("客户端发现差错,传输中止");
    GeneralSend(receive_packet_total,send_packet_total);
}

void ClientSocketHandle::slot_ActionFileErrorReceived(const char *receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    for(auto file_ss_iter = client_file_list_->begin(); file_ss_iter!=client_file_list_->end();++file_ss_iter){
        if (file_ss_iter->session_id==ntohl(receive_packet_header->file_proto.session_id)){
            close(file_ss_iter->file_fd);
            client_file_list_->erase(file_ss_iter);
            break;
        }
    }
    printf("服务端发现差错,传输中止");
    emit sig_ActionGeneralFinishSend(receive_packet_total);
}

void ClientSocketHandle::slot_MenuCacheFlush() {
//    emit sig_MenuCacheServerGet("root\n");
    emit sig_ActionControlServerFileLsSend();
    QString client_data = self_data_->client_data_dir_now;
    client_data+="\n";
    //以下代码是linux限定的..
    DIR *pDir_root;
    struct dirent* ptr;
    if(!(pDir_root = opendir(self_data_->client_data_dir_now))){
        cout<<"Folder doesn't Exist!"<<endl;
        return;
    }
    else{
        client_data += step_MenuCacheFlush(pDir_root,1,self_data_->client_data_dir_now);
    }
//本想用非递归 但是一直做不出来...
/*
    if(!(pDir_root = opendir(self_data_->client_data_dir_now))){
        cout<<"Folder doesn't Exist!"<<endl;
        return;
    }
    else{
        pDir_list.push_back(pDir_root);
        pQString_list.push_back(self_data_->client_data_dir_now);
    }
    auto pQString_p = pQString_list.begin();
    for(auto pDir_p = pDir_list.begin();pDir_p!=pDir_list.end();pDir_p++,pQString_p++){
        ptr = readdir(*pDir_p);
        while(ptr != 0) {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0 && strcmp(ptr->d_name, ".DS_Store") != 0){
                printf("\n%s/%s ", pQString_p->toLatin1().data(),ptr->d_name);
                if(ptr->d_type == DT_DIR){
                    tmp_string = (*pQString_p+"/"+ptr->d_name);
                    pDir_list.push_back(opendir(tmp_string.toLatin1()));
                    pQString_list.push_back(tmp_string);
                    printf("\n[%s is a folder]\n",tmp_string.toLatin1().data());
                }
            }
            ptr = readdir(*pDir_p);
        }
        printf("finish folder %s",pQString_p->toLatin1().data());
        closedir(*pDir_p);
    }
*/

    emit sig_MenuCacheClientGet(client_data);
}

QString ClientSocketHandle::step_MenuCacheFlush(DIR * dir , int deep, const QString &str) {
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
void ClientSocketHandle::slot_ClientStorageConfirm() {
    if(access(client_data_root,R_OK|W_OK|F_OK)<0){
        if(mkdir(client_data_root,S_IRWXU) == 0){
            cout<<"于根目录"<<client_data_root<<"创建了文件夹"<<endl;
        }
        else{
            cout<<"于根目录"<<client_data_root<<"创建文件夹失败"<<endl;
        }
    }
    if(access(self_data_->client_data_dir_now,R_OK|W_OK|F_OK)<0){
        if(mkdir(self_data_->client_data_dir_now,S_IRWXU) == 0){
            cout<<"为用户"<<self_data_->confirmUserName<<"于"<<self_data_->client_data_dir_now<<"创建了文件夹"<<endl;
        }
        else{
            cout<<"试图为用户"<<self_data_->confirmUserName<<"于"<<self_data_->client_data_dir_now<<"创建文件夹失败"<<endl;
        }
    }
    emit hint("目录确认完成");
}

void ClientSocketHandle::slot_ActionControlServerFileLsSend() {
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code = kControlLs;
    send_packet_header->ctl_proto.data_length=0;
    GeneralSend(nullptr,send_packet_total);
}

void ClientSocketHandle::slot_ActionControlServerFileLsReceive(const char * receive_packet_total) {
    auto* receive_packet_header = (header*)receive_packet_total;
    auto* receive_packet_data = (data*)(receive_packet_total+kHeaderSize);
    emit sig_MenuCacheServerGet(QString::fromUtf8(receive_packet_data->ctl_ls.chr));
    emit sig_ActionGeneralFinishSend(receive_packet_total);
}

void ClientSocketHandle::slot_MissionFile(const QString &server_path, const QString &client_path, bool upload,
                                          bool download) {
    if(upload){
        emit sig_ActionFileUploadSend(server_path.toUtf8().data(),client_path.toUtf8().data());
    }
    if(download){
        emit sig_ActionFileDownloadSend(server_path.toUtf8().data(),client_path.toUtf8().data());
    }
}

void ClientSocketHandle::slot_MainEventServerPacket() {
    char receive_message[kHeaderSize + kBufferSize]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(conn_client_fd_, receive_message, kHeaderSize);
    byte_num += read(conn_client_fd_, receive_message + kHeaderSize, ntohl(packet_header->base_proto.data_length));

    if (byte_num > 0) {
        //region *对服务端收包的处理
        switch (packet_header->proto) {
            case kProtoControl:
                switch (packet_header->ctl_proto.ctl_code) {
                    case kControlUnregistered:
                        cout<<("用户名尚未注册...")<<endl;
                        hint("用户名尚未注册...");
                        emit sig_ActionGeneralFinishSend(receive_message);
                        self_data_->state = kHalfLink;
                        return;
                    case kControlRegisterSuccess:
                        cout<<("用户注册成功...")<<endl;
                        hint("用户注册成功...");
                        emit sig_ActionGeneralFinishSend(receive_message);
                        self_data_->state = kHalfLink;
                        return;
                    case kControlDuplicateLogin:
                        cout<<("重复登录...")<<endl;
                        hint("重复登录...");
                        emit sig_ActionGeneralFinishSend(receive_message);
                        self_data_->state = kHalfLink;
                        return;
                    case kControlDuplicateRegister:
                        cout<<("用户名已经注册过，无法注册...")<<endl;
                        hint("用户名已经注册过，无法注册...");
                        emit sig_ActionGeneralFinishSend(receive_message);
                        self_data_->state = kHalfLink;
                        return;
                    case kControlLs:
                        emit sig_ActionControlServerFileLsReceive(receive_message);
                        return;
                    case kControlLogout:
                        emit hint("登出");
                        emit state(kHalfLink);
                        self_data_->state=kHalfLink;
                        emit sig_ActionGeneralFinishSend(receive_message);
                        return;
                    case kControlClientInfo:
                        emit sig_ActionControlClientInfoReceive(receive_message);
                        return;
                    default:
                        cout<<"Invalid CTL packet"<<endl;
                        return;
                }
            case kProtoMessage:
                printf("收到服务器的message包");
                emit sig_EventReceiveMessageFromServer(receive_message);
                return;
            case kProtoChap:
                switch (packet_header->chap_proto.chap_code) {
                    case kChapChallenging:
                        cout<<("[sur-rcv] receive CHAP Challenge\n");
                        emit sig_ActionChapResponse(receive_message);
                        return;
//                        return kProtoChap | kChapChallenging << 2;
                    case kChapSuccess:
                        cout<<("[sur-rcv] CHAP success\n");
                        self_data_->state = kOnline;
                        emit state(kOnline);
                        strcpy(self_data_->client_data_dir_now,client_data_root);
                        strcat(self_data_->client_data_dir_now,self_data_->confirmUserName);
                        emit sig_ClientStorageConfirm();
                        emit sig_ActionGeneralFinishSend(receive_message);
                        return;
                    case kChapFailure:
                        cout<<("[sur-rcv] CHAP failure\n");
                        emit state(kHalfLink);
                        self_data_->state = kHalfLink;
                        emit sig_ActionGeneralFinishSend(receive_message);
                        return;
                    default:
                        cout<<("[sur-rcv] Invalid CHAP_CODE\n");
                        return;
                }
            case kProtoFile:
                switch (packet_header->file_proto.file_code) {
                    case kFileDownload:
                        emit sig_ActionFileDownloadReceive(receive_message);
                        return;
                    case kFileUpload:
                        emit sig_ActionFileUploadReceive(receive_message);
                        return;
                    case kFileTransporting:
                        emit sig_ActionFileTranslatingReceive(receive_message);
                        return ;
                    case kFileAck:
                        emit sig_ActionFileAckReceived(receive_message);
                        return;
                    case kFileEnd:
                        emit sig_ActionFileEndReceived(receive_message);
                        return;
                    case kFileError:
                        emit sig_ActionFileErrorReceived(receive_message);
                        return;
                }
                cout<<("[sur-rcv] invalid File proto [code]...\n");
                return;
            case kProtoGeneralAck:
                emit sig_ActionGeneralAckReceive(receive_message);
                return;
//                return kProtoGeneralAck;
            case kProtoGeneralFinish:
                emit sig_ActionGeneralFinishReceive(receive_message);
                return;
//                return kProtoGeneralFinish;
            default:
                printf("[sur-rcv] Invalid proto,with number: %d,",packet_header->proto);
                cout<<"drop the packet...";
//                cue_flag = true;
//                return -1;
        }
        //endregion

    }
    else if(byte_num < 0){
        cout<<("[sur-rcv] receive error!\n");
        return;
    }
    else{
        FD_CLR(conn_client_fd_, client_fd_set_);
        cout<<"server("<<conn_client_fd_<<") exit!"<<endl;
        emit sig_EventServerShutdown();
    }
}



























//*********************************//

ClientSocketThread::ClientSocketThread(QThread *parent,ClientSocketHandle* client_socket_handle):
QThread(parent),timer_(new QTimer()){
    if (client_socket_handle==nullptr){
        client_socket_handle_ = new ClientSocketHandle;
    }
    else{
        client_socket_handle_ = client_socket_handle;
    }
    timer_->setSingleShot(true);
}

ClientSocketThread::~ClientSocketThread() {
    delete timer_;
    delete client_socket_handle_;
}

void ClientSocketThread::run() {
    QObject::connect(timer_,&QTimer::timeout,client_socket_handle_,&ClientSocketHandle::slot_timeout);
    QObject::connect(client_socket_handle_,&ClientSocketHandle::sig_mission_finish,
                     this, &ClientSocketThread::slot_turn_end);
    exec();
}

void ClientSocketThread::slot_timer_ready() {
    flag_timer_running_= true;
    timer_->start(1);
}

void ClientSocketThread::slot_turn_end(bool) {
    if(flag_timer_running_){
        timer_->start(1);
    }
}

void ClientSocketThread::stop_timer() {
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
        printf("过长时间没有回应(%d倍初始计时)，定时器%d自毁...\n",(1<<retry_max_)-1,timer_id_);
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



