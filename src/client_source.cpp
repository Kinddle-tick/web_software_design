//
// Created by Kinddle Lee on 2021/11/27.
//

#include "client_source.h"
using namespace std;

//region help文档
HelpDoc client_help[]={
        {"login",
                "login            连接到服务器并使用账号密码登录",
                "login <username> <password>\n*登录成功与否会给予反馈"},

        {"logout",
                "logout           登出",
                "logout"},

        {"register",
                "register         连接到服务器并尝试使用账号密码注册",
                "register <username> <password>\n*注册成功与否会给予反馈"},

        {"msg",
                "msg              发送一条信息到服务器，默认广播给所有人",
                "msg [-t <username>] <your message>\n*发送消息 如果消息包括空格请用双引号(英文)包围"},

        {"change_password",
                "change_password  更改密码",
                "change_password <old_password> <new_password>"},

        {"change_username",
                "change_username  更改用户名（若数据库没有重复的用户名）",
                "change_username <new username>\n*登录后方可修改，修改成功与否会给予反馈"},

        {"ls",
                "ls               查看服务器自己文件夹的内容",
                "ls 查看当前目录下的内容"},

        {"cd",
                "cd               进入指定目录",
                "cd path"},

        {"download",
                "download         下载指定文件到本地",
                "download <file_path>\n*成功与否会给予反馈"},

        {"upload",
                "upload           上传指定文件",
                "upload <file_path>\n*成功与否会给予反馈"},
        {"monitor",
                "monitor           查看该用户当前传输文件的相关信息",
                "monitor [session_id]"},
        {},//结尾的判据
};
//endregion

bool ErrorSimulator(int prob=0){
    if(random()%100 < prob){
        return true;
    }
    else{
        return false;
    }
}

ssize_t EventServerMessage() {
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
    char receive_message[kHeaderSize + kBufferSize]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(conn_client_fd, receive_message, kHeaderSize);
    byte_num += read(conn_client_fd, receive_message + kHeaderSize, ntohl(packet_header->base_proto.data_length));

    if (byte_num > 0) {
        //region *对服务端收包的处理
        switch (packet_header->proto) {
            case kProtoControl:
                switch (packet_header->ctl_proto.ctl_code) {
                    case kControlUnregistered:
                        cout<<("用户名尚未注册...")<<endl;
                        self_data->state = kOffline;
                        cue_flag = true;
                        return kProtoControl | kControlUnregistered << 2;
                    case kControlLs:
                        BaseActionInterrupt();
                        cout<<"\n"<<(((data *)(receive_message + kHeaderSize))->ctl_ls.chr);
                        cue_flag = true;
                        return kProtoControl | kControlLs << 2;
                    default:
                        cout<<"Invalid CTL packet"<<endl;
                        return -1;
                }
            case kProtoMessage:
                BaseActionInterrupt();
                ActionReceiveMessageFromServer(receive_message);
                cue_flag = true;
                return kProtoMessage;
            case kProtoChap:
                switch (packet_header->chap_proto.chap_code) {
                    case kChapChallenging:
                        ActionChapResponse(receive_message);
                        cout<<("[sur-rcv] receive CHAP Challenge\n");
                        return kProtoChap | kChapChallenging << 2;
                    case kChapSuccess:
                        cout<<("[sur-rcv] CHAP success\n");
                        self_data->state = kOnline;
                        strcpy(self_data->confirmUserName,self_data->tmpUserName);
                        strcpy(self_data->client_data_dir_now,self_data->client_data_root);
                        strcat(self_data->client_data_dir_now,self_data->confirmUserName);
                        cue_flag = true;
                        ActionGeneralFinishSend(receive_message);
                        return kProtoChap | kChapSuccess << 2;
                    case kChapFailure:
                        cout<<("[sur-rcv] CHAP failure\n");
                        self_data->state = kOffline;
                        cue_flag = true;
                        ActionGeneralFinishSend(receive_message);
                        return kProtoChap | kChapFailure << 2;
                    default:
                        cout<<("[sur-rcv] Invalid CHAP_CODE\n");
                        cue_flag = true;
                        return -1;
                }
            case kProtoFile:
                switch (packet_header->file_proto.file_code) {
                    case kFileTransporting:
                        ActionFileTransportingReceived(receive_message);
                        return kProtoFile | kFileTransporting << 2;
                    case kFileResponse:
                        BaseActionInterrupt();
                        cout<<endl;
                        ActionFileResponseReceived(receive_message);
                        return kProtoFile | kFileTransporting << 2;
                    case kFileEnd:
                        ActionFileEndReceived(receive_message);
                        cue_flag = true;
                        return kProtoFile | kFileEnd << 2;
                }
                cout<<("[sur-rcv] FILE Request..?\n");
                cue_flag = true;
                return kProtoFile;
            case kProtoGeneralAck:
                ActionGeneralAckReceive(receive_message);
                return kProtoGeneralAck;
            case kProtoGeneralFinish:
                ActionGeneralFinishReceive(receive_message);
                return kProtoGeneralFinish;
            default:
                printf("[sur-rcv] Invalid proto,with number: %d,",packet_header->proto);
                cout<<"drop the packet...";
                cue_flag = true;
                return -1;
        }
        //endregion

    }
    else if(byte_num < 0){
        cout<<("[sur-rcv] receive error!\n");
        return -1;
    }
    else{
        FD_CLR(conn_client_fd, client_fd_set);
        cout<<"server("<<conn_client_fd<<") exit!"<<endl;
        // 广播到所有客户端
        strcpy(receive_message + kHeaderSize, "Server Exit!");
        for(auto & client_point : *gui_client_list){
            BaseActionInterrupt();
            cout<<("\n**server exit")<<endl;
            cue_flag = true;
            packet_header->base_proto.data_length = htonl(strlen(receive_message + kHeaderSize));
            send(client_point.fd, receive_message + kHeaderSize, ntohl(packet_header->base_proto.data_length), 0);
        }
        return kServerShutdown;
    }
}

int EventNewGui(){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: EventNewGui"<<endl;
#endif
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(gui_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        char nickname_tmp[21]{0};
        snprintf(nickname_tmp,20,"GUI[unknown]");
        FdInfo tmp_client{client_sock_fd};
        strcpy(tmp_client.nickname,nickname_tmp);
        gui_client_list->push_back(tmp_client);
        cout<<"GUI("<<tmp_client.nickname<<") joined!"<<endl;
    }
    return 0;
}

ssize_t EventGuiMessage(FdInfo* client){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: EventGUIMsg"<<endl;
#endif
    char recv_msg[kHeaderSize + kBufferSize];
    bzero(recv_msg, kHeaderSize + kBufferSize);
    ssize_t byte_num=read(client->fd, recv_msg + kHeaderSize, kBufferSize);
    if(byte_num >0){
        cout << "message form socket(" << client->nickname << "):" << recv_msg + kHeaderSize << endl;
        send(conn_client_fd, recv_msg, strlen(recv_msg + kHeaderSize) + kHeaderSize + 1, 0);
    }
    else if(byte_num < 0){
        cout<<("recv error!");
    }else{
        cout<<"socket("<<client->nickname<<") exit! \nand the Procedure is exiting..."<<endl;
        FD_CLR(client->fd, client_fd_set);
        close(client->fd);
    }
    return byte_num;
}

int EventStdinMessage(){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
    /** region * 检查<标准输入>是否存在于ser_fdset集合中（也就是说，检测到键盘输入时，做如下事情） ## */
    char input_msg[kHeaderSize + kBufferSize]={0};
    char* para[128]={nullptr};//顶天接受128个用空格分开的参数了
    unsigned int para_count=0;
    bool blank_flag= true;
    bool in_txt_flag = false;
    cin.getline(input_msg, kHeaderSize + kBufferSize, '\n');
    //特判：
    if(strcmp(input_msg,"exit")==0){
        self_data->state = kOffline;
        return kClientExit;
    }
    if(strlen(input_msg)==0){
        // 关于cin.eofbit何时置位的问题 https://en.cppreference.com/w/cpp/io/ios_base/iostate
        // cin.badbit是非常严重的错误，（比如空指针..） 区区后台运行根本不会导致
        if(cin.rdstate()&(istream::eofbit|istream::failbit)){//当读取失败或者读取到达流尾
            cout<<("发现处于后台工作 stdin被抑制...")<<endl;
            return kStdinExit;
        }
        return kNormal;
    }
    //根据空格分离参数 para中将会保存每一个参数的指针 para_count则为参数的个数
    for(char * chr_ptr=input_msg;*chr_ptr != 0;chr_ptr++){
        if(*chr_ptr=='\"'){
            if(chr_ptr == input_msg||*(chr_ptr-1)!='\\'){
                *chr_ptr=0;
                in_txt_flag=!in_txt_flag;
                continue;
            }
        }
        if(*chr_ptr==' ' && !in_txt_flag){
            blank_flag = true;
            *chr_ptr=0;
        } else if (blank_flag){
            blank_flag = false;
            para[para_count]=chr_ptr;
            para_count++;
        }
    }

    //region 指令的具体实现部分
    if(!strcmp(para[0],"help")){
        ActionPrintConsoleHelp(para[1]);
    }
    else if (!strcmp(para[0],"login")){
        if(para_count > 2){
            self_data->password = (unsigned int)strtoul(para[2],nullptr,10);
        }
        if(para_count >1){
            strcpy(self_data->tmpUserName,para[1]);
        }else{
            strcpy(self_data->tmpUserName,self_data->confirmUserName);
        }
        ActionConnectToServer();
        ActionControlLogin();
        return kConnectServer;
    }
    else if (!strcmp(para[0],"register")){
        cout<<"register 未实现";
    }
    else if (!strcmp(para[0],"msg")){
        int rtn = ActionSendMessageToServer(para[1]);
        if(rtn == -1){
            return kOtherError;
        }else{
            return kNormal;
        }
    }
    else if (!strcmp(para[0],"logout")){
        if(conn_client_fd>0){
            close(conn_client_fd);
            FD_CLR(conn_client_fd,client_fd_set);
            conn_client_fd=0;
            self_data->state = kOffline;
        }
        return kNormal;
    }
    else if (!strcmp(para[0],"change_password")){
        cout<<"change_password";
    }
    else if (!strcmp(para[0],"change_username")){
        cout<<"change_username";
    }
    else if (!strcmp(para[0],"ls")){
        ActionControlLs();
        return kNormal;
    }
    else if (!strcmp(para[0],"cd")){
        cout<<"cd";
    }
    else if (!strcmp(para[0],"download")){
        if(para_count < 2){
            cout<<"请输入文件名..."<<endl;
            return kNormal;
        }
        ActionFileRequestSend(para[1]);
        return kNormal;
    }
    else if (!strcmp(para[0],"upload")){
        cout<<"upload";
    }
    else if (!strcmp(para[0],"monitor")){
        ActionControlMonitor();
        return kNormal;
    }
    else{
        cout<<"Invalid command:,'"<<para[0]<<R"('try to use 'help')"<<endl;
        return kNormal;
    }
    //endregion
    cout<<"这个命令也许还没有完成捏!"<<endl;
    return kNormal;
    /** endregion */
}

void BaseActionInterrupt(){
    cout<<"\033[31m<interrupt⬇>\033[0m";
}


int ActionGeneralFinishSend(const char * receive_packet_total){
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralFinish,
                                          .detail_code =0,
                                          .timer_id_tied = client_timer_id = client_timer_id % 255 + 1,
                                          .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
                                          .data_length=htonl(0),
                                          }};
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header.base_proto.timer_id_tied,conn_client_fd,
                                                    (const char *)&send_packet_header,kHeaderSize+ntohl(send_packet_header.chap_proto.data_length));
    tmp_timer->set_retry_max(3);
    timer_list->push_back(tmp_timer);
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(conn_client_fd,&send_packet_header,kHeaderSize+ntohl(send_packet_header.base_proto.data_length),0);
    }
    return 0;
}
int ActionGeneralFinishReceive(const char * receive_packet_total){
    ActionGeneralAckSend(receive_packet_total);
    return 0;
}

//通用ACK报文 因为Ack报文是不建议开定时器重发的（因为没有终止定时器的判断依据），所以如果Ack报文真的一直发不出去，那服务端一直请求也是正常的
int ActionGeneralAckSend(const char * receive_packet_total){
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralAck,
                                          .detail_code = receive_packet_header->base_proto.detail_code,
                                          .timer_id_tied = 0,
                                          .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
                                          .data_length = htonl(0)}};
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd);
    if(!ErrorSimulator(kGenericAckProb)){
        send(conn_client_fd, &send_packet_header, kHeaderSize + ntohl(send_packet_header.base_proto.data_length), 0);
    }
    return 0;
}
int ActionGeneralAckReceive(const char * receive_packet_total){
    auto* receive_packet_header = (header*)receive_packet_total;
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,conn_client_fd);
    return 0;
}

int ActionPrintConsoleHelp(const char* first_para = nullptr){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: ActionPrintConsoleHelp"<<endl;
#endif
    cout<<"[help]: ";
    if(first_para==nullptr){
        //region 可用的指令
        cout<<"可使用的指令如下:(区分大小写)\n";
        for(int i = 0;client_help[i].cmd_name!= nullptr;i++){
            cout<<client_help[i].cmd_description<<endl;
        }
        cout<<"输入 help [命令符号]查看每条指令的具体使用方法"<<endl;
        return 0;
    }
    else{
        for(int i = 0;client_help[i].cmd_name!= nullptr;i++){
            if(!strcmp(first_para,client_help[i].cmd_name)){
                cout<<client_help[i].cmd_pattern<<endl;
                return 0;
            }
        }
        cout<<"该指令无效"<<endl;
        return 1;
    }
}

int ActionChapResponse(const char* receive_packet_total){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: ActionCHAPResponse"<<endl;
#endif
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

    memcpy(send_packet_data->chap_response.userName, self_data->tmpUserName, kUsernameLength);
    send_packet_data->chap_response.answer = htonl(answer^self_data->password);
//    send_packet_header->chap_proto.data_length = htonl(sizeof(send_packet_data->chap_response));

    send_packet_header->chap_proto.timer_id_tied = client_timer_id = client_timer_id%255+1;
    send_packet_header->chap_proto.timer_id_confirm = receive_packet_header->chap_proto.timer_id_tied;
    auto* tmp_timer = new TimerRetranslationSession(kGenericTimeInterval,send_packet_header->chap_proto.timer_id_tied,conn_client_fd,
                                                    send_packet_total,kHeaderSize + ntohl(send_packet_header->chap_proto.data_length));
    timer_list->push_back(tmp_timer);
    TimerDisable(receive_packet_header->chap_proto.timer_id_confirm,conn_client_fd);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(conn_client_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->chap_proto.data_length), 0);
    }
    return 0;
}

int ActionConnectToServer(){
#if CLIENT_DEBUG_LEVEL >3
    cout<<"[DEBUG]: ActionConnectServer"<<endl;
#endif
    sockaddr_in server_addr{};
    // 面向服务器的链接 在本程序中是作为客户端存在的
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(kServerPort);
    server_addr.sin_addr.s_addr =INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if(conn_client_fd>0){
        close(conn_client_fd);
        FD_CLR(conn_client_fd,client_fd_set);
    }
    conn_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(conn_client_fd == -1){
        perror("conn_client_fd socket error");
        self_data->state = kOffline;
        return 1;
    }
    if(connect(conn_client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
    {
        //add server
        FD_SET(conn_client_fd, client_fd_set);
        if(max_fd < conn_client_fd)
        {
            max_fd = conn_client_fd;
        }
        self_data->state = kLoginTry;
        return 0;
    } else {
        self_data->state = kOffline;
        return -1;
    }
}

int ActionControlLogin(){
    if(self_data->state != kOffline){
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        send_packet_header->proto = kProtoControl;
        send_packet_header->ctl_proto.ctl_code = kControlLogin;
        memcpy(send_packet_data->ctl_login.userName, self_data->tmpUserName, kUsernameLength);
        send_packet_header->ctl_proto.data_length = htonl(sizeof(send_packet_data->ctl_login));

        //region 范式
        send_packet_header->ctl_proto.timer_id_tied= client_timer_id = client_timer_id % 255 + 1;
        send_packet_header->ctl_proto.timer_id_confirm = 0;//作为流程中第一个触发的报文 不会存在在另一侧绑定的报文数字
        auto* tmp_timer=new TimerRetranslationSession(kGenericTimeInterval, send_packet_header->ctl_proto.timer_id_tied, conn_client_fd, send_packet_total,
                                                      kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length));
        timer_list->push_back(tmp_timer);
        TimerDisable(0,conn_client_fd);
        if(!ErrorSimulator(kGenericErrorProb)){
            send(conn_client_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
        }
        //endregion

        self_data->state = kLoginTry;
        return 0;
    } else{
        cout<< "can not send <login> packet to server in state <kOffline>" <<endl;
        return 1;
    }

}

int ActionControlLs(){
    if(self_data->state == kOnline){
        char packet_total[kHeaderSize + kBufferSize]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total + kHeaderSize);
        packet_header->proto=kProtoControl;
        packet_header->ctl_proto.ctl_code = kControlLs;
        packet_header->ctl_proto.data_length = htonl(0);
        send(conn_client_fd, packet_total, kHeaderSize + ntohl(packet_header->ctl_proto.data_length), 0);
        return 0;
    }
    else{
        cout<< "can only send ls to server in state <kOnline>" <<endl;
        return -1;
    }
}

int ActionControlMonitor(){
    return 0;
}

int ActionFileRequestSend(const char* file_path){
    if(self_data->state == kOnline){
        char packet_total[kHeaderSize + kBufferSize]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total + kHeaderSize);
        packet_header->proto=kProtoFile;
        packet_header->file_proto.file_code = kFileRequest;

        strcpy(packet_data->file_request.file_path,file_path);
        packet_header->file_proto.data_length = htonl(strlen(file_path)+sizeof(packet_data->file_request.init_time));
        send(conn_client_fd, packet_total, kHeaderSize + ntohl(packet_header->file_proto.data_length), 0);
        return 0;
    }
    else{
        cout<< "can only send download cmd to server in state <kOnline>" <<endl;
        return -1;
    }
}

int ActionFileResponseReceived(const char * received_packet_total){
    auto* received_packet_header = (header *)received_packet_total;
    if(self_data->state == kOnline){
        auto* received_packet_data = (data *)(received_packet_total + kHeaderSize);
        char request_path[kUserPathMaxLength]={0};
        strcpy(request_path,self_data->client_data_dir_now);
        if(access(request_path,W_OK)!=0){
            cout<<request_path<<"目录不存在，将会被创建"<<endl;
            mkdir(request_path,S_IRWXU);
        }
        strcat(request_path,"/");
        strcat(request_path,received_packet_data->file_response.file_path);
        if(access(request_path,W_OK)==0){
            cout<<request_path<<"文件已存在，将会被清除"<<endl;
        }
        int tmp_fd = open(request_path,O_WRONLY|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR|S_IXUSR);
        if(tmp_fd<0){
            perror("文件打开失败");
            return -1;
        }
        FileSession tmp_session{.session_id=ntohl(received_packet_header->file_proto.session_id),
                .sequence = ntohl(received_packet_header->file_proto.sequence),
                .file_fd = tmp_fd,
                .init_time = ntohl(received_packet_data->file_response.init_time),
                .init_sequence = ntohl(received_packet_header->file_proto.sequence)};
        file_list->push_back(tmp_session);
        ActionFileAckSend(tmp_session.session_id, tmp_session.sequence);
        cout<<"已准备好接受文件传输"<<endl;
        return 0;
    }
    else{
        cout<<"需要在线才能进行文件传输"<<endl;
        ActionFileErrorSend(ntohl(received_packet_header->file_proto.session_id),
                            ntohl(received_packet_header->file_proto.sequence));
        return -1;
    }
}

int ActionFileTransportingReceived(const char* received_packet_total){
    auto* received_packet_header = (header*)received_packet_total;
    auto* received_packet_data = (data*)(received_packet_total + kHeaderSize);
    for(auto &file_ss_iter :*file_list){
        if (file_ss_iter.session_id==ntohl(received_packet_header->file_proto.session_id)){
            if(file_ss_iter.sequence == ntohl(received_packet_header->file_proto.sequence)){
                ssize_t size = write(file_ss_iter.file_fd,received_packet_data->file_transport.file_data,
                                     ntohl(received_packet_header->file_proto.frame_transport_length));
                if(size >0){
                    file_ss_iter.sequence+=size;
//                    printf("%ld",clock()-file_ss_iter.init_time);
//                    fprintf(logger_fptr,"fd:%ld完成了一轮确收",clock()-file_ss_iter.init_time);
//                    cout<<"session:"<<ntohl(received_packet_header->file_proto.session_id)<<" 完成了一次确收"<<endl;
                    ActionFileAckSend(file_ss_iter.session_id,
                                      file_ss_iter.sequence);
                    return 0;
                }
                else{
                    ActionFileErrorSend(file_ss_iter.session_id,
                                        file_ss_iter.sequence);
                    return -1;
                }
            }
            cout<<"找到对应的session,但是sequence不相同"<<endl;
        }
    }
    cout<<"没有找到session和sequence均符合条件的会话;"<<endl;
    return -1;
}

int ActionFileAckSend(uint32_t session_id= 0, uint32_t sequence= 0){
    header send_packet_header{.file_proto{.file_code=kFileAck,.frame_transport_length=htonl(0),
                                          .session_id=htonl(session_id),.sequence=htonl(sequence)}};
    send_packet_header.file_proto.data_length = htonl(0);
    send(conn_client_fd,&send_packet_header + ntohl(send_packet_header.file_proto.data_length), kHeaderSize, 0);
    return 0;
}

int ActionFileErrorSend(uint32_t session_id=0,uint32_t sequence_id=0){
    cout<<"fileErrorSend"<<endl;
    return 0;
}

int ActionFileEndReceived(const char* received_packet_total){
    auto* received_packet_header = (header*)received_packet_total;
    cout<<"一个文件接受完了"<<endl;
    for(auto file_ss_iter = file_list->begin(); file_ss_iter!=file_list->end();++file_ss_iter){
        if(file_ss_iter->session_id == ntohl(received_packet_header->file_proto.session_id)){
            close(file_ss_iter->file_fd);
            file_list->erase(file_ss_iter);
            return 0;
        }
    }
    cout<<"没有在列表里找到相应的文件"<<endl;
    return -1;
}

int ActionSendMessageToServer(const char* message){
    if(self_data->state == kOnline) {
        char send_message_packet[kHeaderSize + kBufferSize]={0};
        auto * send_message_header=(header*)send_message_packet;
        auto * send_message_data = (data *)(send_message_packet + kHeaderSize);

        send_message_header->msg_proto.proto = kProtoMessage;
        strcpy(send_message_data->msg_general.msg_data, message);

        cout << "[send->server] message send to server:" << send_message_data->msg_general.msg_data << endl;
        send_message_header->msg_proto.data_length = htonl(strlen(send_message_data->msg_general.msg_data)
                                                     + sizeof(send_message_data->msg_general.userName));
        send(conn_client_fd, send_message_packet, kHeaderSize + ntohl(send_message_header->msg_proto.data_length), 0);
        cue_flag = true;
        return 0;
    }
    else{
        cout<<"[send->server] Error: please connect server..."<<endl;
        cue_flag = true;
        return -1;
    }
}

int ActionReceiveMessageFromServer(const char* receive_packet_total){
    auto* packet_header = (header*)receive_packet_total;
    auto* packet_data  = (data*)(receive_packet_total + kHeaderSize);
    cout<<"\n[sur-rcv] message form server("<<conn_client_fd<<"):\033[34m"<<packet_data->msg_general.msg_data<<"\033[0m"<<endl;

    // 广播到所有客户端（gui）
    for (auto & client_point : *gui_client_list) {
        cout<<"[sur-rcv] send message to client("<<client_point.nickname<<").socket_fd = "<<client_point.fd<<endl;
        packet_header->base_proto.data_length = htonl(strlen(packet_data->msg_general.msg_data)+sizeof(UserNameString));
        send(client_point.fd, receive_packet_total, kHeaderSize + ntohl(packet_header->base_proto.data_length), 0);
    }
    return 0;
}

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

TimerRemoveSession::TimerRemoveSession(double_t timing_interval, uint8_t timer_id, SocketFileDescriptor socket_fd, bool(*function_point)()):
        TimerSession(timing_interval, timer_id, socket_fd), trigger_void_function_(function_point){
    timer_state_ = kTimerWorking;
}
bool TimerRemoveSession::TimerTrigger() {
    if(timer_state_ == kTimerWorking){
//        TimerUpdate();
        return (*trigger_void_function_)();
    }
    else return false;
}
//endregion

bool TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd){
    if(timer_id==0){
        return false;//特判 因为经常用
    }
    for(auto &iter :*timer_list){
        if(iter->timer_id_==timer_id){
            if(iter->socket_fd_ == socket_fd){
                return iter->TimerDisable();
            }
        }
    }
    return false;
}


