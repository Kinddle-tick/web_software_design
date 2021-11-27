//
// Created by Kinddle Lee on 2021/11/27.
//

#include "client_source.h"
using namespace std;

//region help文档
help_doc client_help[]={
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

ssize_t EventServerMsg() {
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
    char receive_message[HEADER_SIZE + BUFFER_SIZE]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(conn_client_fd, receive_message, HEADER_SIZE);
    byte_num += read(conn_client_fd,receive_message+HEADER_SIZE,packet_header->base_proto.data_length);

    if (byte_num > 0) {
        //region *对服务端收包的处理
        switch (packet_header->proto) {
            case ProtoCTL:
                switch (packet_header->ctl_proto.ctl_code) {
                    case CTLUnregistered:
                        cout<<("用户名尚未注册...")<<endl;
                        self_data->state = Offline;
                        cue_flag = true;
                        return ProtoCTL|CTLUnregistered<<2;
                    case CTLLs:
                        BaseActionInterrupt();
                        cout<<"\n"<<(((data *)(receive_message+HEADER_SIZE))->ctl_ls.chr);
                        cue_flag = true;
                        return ProtoCTL|CTLLs<<2;
                    default:
                        cout<<"Invalid CTL packet"<<endl;
                        return -1;
                }
                break;
            case ProtoMsg:
                BaseActionInterrupt();
                ActionMessageFromServer(receive_message);
                cue_flag = true;
                return ProtoMsg;
            case ProtoCHAP:
                switch (packet_header->chap_proto.chap_code) {
                    case CHAPChallenging:
                        ActionCHAPResponse(receive_message);
                        cout<<("[sur-rcv] receive CHAP Challenge\n");
                        return ProtoCHAP|CHAPChallenging<<2;
                    case CHAPSuccess:
                        cout<<("[sur-rcv] CHAP success\n");
                        self_data->state = Online;
                        strcpy(self_data->confirmUserName,self_data->tmpUserName);
                        strcpy(self_data->client_data_dir_now,self_data->client_data_root);
                        strcat(self_data->client_data_dir_now,self_data->confirmUserName);
                        cue_flag = true;
                        return ProtoCHAP|CHAPSuccess<<2;
                    case CHAPFailure:
                        cout<<("[sur-rcv] CHAP failure\n");
                        self_data->state = Offline;
                        cue_flag = true;
                        return ProtoCHAP|CHAPFailure<<2;
                    default:
                        cout<<("[sur-rcv] Invalid CHAP_CODE\n");
                        cue_flag = true;
                        return -1;
                }
                break;
            case ProtoFile:
                switch (packet_header->file_proto.file_code) {
                    case FileTransporting:
                        ActionFileTransportingReceived(receive_message);
                        return ProtoFile|FileTransporting<<2;
                    case FileResponse:
                        BaseActionInterrupt();
                        cout<<endl;
                        ActionFileResponseReceived(receive_message);
                        return ProtoFile|FileTransporting<<2;
                    case FileEnd:
                        ActionFileEndReceived(receive_message);
                        cue_flag = true;
                        return ProtoFile|FileEnd<<2;
                }
                cout<<("[sur-rcv] FILE Request..?\n");
                cue_flag = true;
                return ProtoFile;
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
        strcpy(receive_message+HEADER_SIZE,"Server Exit!");
        for(auto & client_point : *gui_client_list){
            BaseActionInterrupt();
            cout<<("\n**server exit")<<endl;
            cue_flag = true;
            packet_header->base_proto.data_length = strlen(receive_message+HEADER_SIZE);
            send(client_point.fd, receive_message+HEADER_SIZE, packet_header->base_proto.data_length, 0);
        }
        return kServerShutdown;
    }
    return -1;
}

int EventNewGui(){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventNewGui"<<endl;
#endif
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(gui_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        char nickname_tmp[21]{0};
        snprintf(nickname_tmp,20,"GUI[unknown]");
        auto * tmp_client = new fd_info{client_sock_fd};
        strcpy(tmp_client->nickname,nickname_tmp);
        gui_client_list->push_back(*tmp_client);
        cout<<"GUI("<<tmp_client->nickname<<") joined!"<<endl;
    }
    return 0;
}

ssize_t EventGUIMsg(fd_info* client){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventGUIMsg"<<endl;
#endif
    char recv_msg[HEADER_SIZE+BUFFER_SIZE];
    bzero(recv_msg,HEADER_SIZE+BUFFER_SIZE);
    ssize_t byte_num=read(client->fd,recv_msg+HEADER_SIZE,BUFFER_SIZE);
    if(byte_num >0){
        cout<<"message form socket("<<client->nickname<<"):"<<recv_msg+HEADER_SIZE<<endl;
        send(conn_client_fd, recv_msg, strlen(recv_msg + HEADER_SIZE) + HEADER_SIZE + 1, 0);
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

int EventStdinMsg(){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
    /** region * 检查<标准输入>是否存在于ser_fdset集合中（也就是说，检测到键盘输入时，做如下事情） ## */
    char input_msg[HEADER_SIZE+BUFFER_SIZE]={0};
    char* para[128]={nullptr};//顶天接受128个用空格分开的参数了
    unsigned int para_count=0;
    bool blank_flag= true;
    bool in_txt_flag = false;
    cin.getline(input_msg,HEADER_SIZE+BUFFER_SIZE,'\n');
    //特判：
    if(strcmp(input_msg,"exit")==0){
        self_data->state = Offline;
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
        ActionConnectServer();
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
            self_data->state = Offline;
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
        ActionFileRequest(para[1]);
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

int ActionPrintConsoleHelp(const char* first_para = nullptr){
#if DEBUG_LEVEL >0
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
};

int ActionCHAPResponse(const char* packet_total){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: ActionCHAPResponse"<<endl;
#endif
    auto* packet_header=(header*)packet_total;
    int one_data_size = packet_header->chap_proto.one_data_size;
    uint answer = 0;
    union num{
        int32_t int32;
        uint32_t uint32;
        uint16_t uint16;
        uint8_t uint8;
    };
    for(int i = 0;i<packet_header->chap_proto.number_count; i++){
        num tmp_num{};
        memcpy(&tmp_num, packet_total + HEADER_SIZE + i * (1 << one_data_size), 1 << one_data_size);
        answer += one_data_size==0 ? tmp_num.uint8 : 0;
        answer += one_data_size==1 ? ntohs(tmp_num.uint16) : 0;
        answer += one_data_size==2 ? ntohl(tmp_num.uint32) : 0;
    }

    char send_packet_total[HEADER_SIZE+BUFFER_SIZE];
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data=(data*)(send_packet_total+HEADER_SIZE);
    memcpy(send_packet_header,packet_header,HEADER_SIZE);
    send_packet_header->chap_proto.chap_code=CHAPResponse;
    send_packet_header->chap_proto.sequence+=1;
    memcpy(send_packet_data->chap_response.userName, self_data->tmpUserName,USERNAME_LENGTH);
    send_packet_data->chap_response.answer = htonl(answer^self_data->password);
    send_packet_header->chap_proto.data_length = sizeof(send_packet_data->chap_response);
    send(conn_client_fd,send_packet_total, HEADER_SIZE+send_packet_header->chap_proto.data_length,0);
    return 0;
};

int ActionConnectServer(){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: ActionConnectServer"<<endl;
#endif
    sockaddr_in server_addr{};
    // 面向服务器的链接 在本程序中是作为客户端存在的
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SER_PORT);
    server_addr.sin_addr.s_addr =INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);

    if(conn_client_fd>0){
        close(conn_client_fd);
        FD_CLR(conn_client_fd,client_fd_set);
    }
    conn_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(conn_client_fd == -1){
        perror("conn_client_fd socket error");
        self_data->state = Offline;
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
        self_data->state = LoginTry;
        return 0;
    } else {
        self_data->state = Offline;
        return -1;
    }
}

int ActionControlLogin(){
    if(self_data->state != Offline){
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto = ProtoCTL;
        packet_header->ctl_proto.ctl_code = CTLLogin;
        memcpy(packet_data->ctl_login.userName, self_data->tmpUserName,USERNAME_LENGTH);
        packet_header->ctl_proto.data_length = sizeof(packet_data->ctl_login);
        send(conn_client_fd,packet_total,HEADER_SIZE+packet_header->ctl_proto.data_length,0);
        self_data->state = LoginTry;
        return 0;
    } else{
        cout<< "can not send <login> packet to server in state <Offline>" <<endl;
        return 1;
    }

}

int ActionControlLs(){
    if(self_data->state == Online){
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto=ProtoCTL;
        packet_header->ctl_proto.ctl_code = CTLLs;
        packet_header->ctl_proto.data_length = 0;
        send(conn_client_fd,packet_total,HEADER_SIZE+packet_header->ctl_proto.data_length,0);
        return 0;
    }
    else{
        cout<< "can only send ls to server in state <Online>" <<endl;
        return -1;
    }
}

int ActionControlMonitor(){
    return 0;
}

int ActionFileRequest(const char* file_path){
    if(self_data->state == Online){
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto=ProtoFile;
        packet_header->file_proto.file_code = FileRequest;

        strcpy(packet_data->file_request.file_path,file_path);
        packet_header->file_proto.data_length = strlen(file_path)+sizeof(packet_data->file_request.init_clock);
        send(conn_client_fd,packet_total,HEADER_SIZE+packet_header->file_proto.data_length,0);
        return 0;
    }
    else{
        cout<< "can only send download cmd to server in state <Online>" <<endl;
        return -1;
    }
}

int ActionFileResponseReceived(const char * received_packet_total){
    auto* received_packet_header = (header *)received_packet_total;
    if(self_data->state == Online){
        auto* received_packet_data = (data *)(received_packet_total+HEADER_SIZE);
        char request_path[USER_PATH_MAX_LENGTH]={0};
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
        file_session tmp_session{.session_id=received_packet_header->file_proto.session_id,
                .sequence = received_packet_header->file_proto.sequence,
                .file_fd = tmp_fd,
                .init_clock = received_packet_data->file_response.init_clock,
                .init_sequence = received_packet_header->file_proto.sequence};
        file_list->push_back(tmp_session);
        ActionFileACKSend(tmp_session.session_id,tmp_session.sequence);
        cout<<"已准备好接受文件传输"<<endl;
        return 0;
    }
    else{
        cout<<"需要在线才能进行文件传输"<<endl;
        ActionFileErrorSend(received_packet_header->file_proto.session_id,received_packet_header->file_proto.sequence);
        return -1;
    }
}

int ActionFileTransportingReceived(const char* received_packet_total){
    auto* received_packet_header = (header*)received_packet_total;
    auto* received_packet_data = (data*)(received_packet_total+HEADER_SIZE);
    for(auto &file_ss_iter :*file_list){
        if (file_ss_iter.session_id==received_packet_header->file_proto.session_id){
            if(file_ss_iter.sequence == received_packet_header->file_proto.sequence){
                ssize_t size = write(file_ss_iter.file_fd,received_packet_data->file_transport.file_data,
                                     received_packet_header->file_proto.frame_transport_length);
                if(size >0){
                    file_ss_iter.sequence+=size;
//                    printf("%ld",clock()-file_ss_iter.init_clock);

//                    fprintf(logger_fptr,"fd:%ld完成了一轮确收",clock()-file_ss_iter.init_clock);
//                    cout<<"session:"<<received_packet_header->file_proto.session_id<<" 完成了一次确收"<<endl;
                    ActionFileACKSend(received_packet_header->file_proto.session_id,
                                      received_packet_header->file_proto.sequence+size);
                    return 0;
                }
                else{
                    ActionFileErrorSend(received_packet_header->file_proto.session_id, received_packet_header->file_proto.sequence);
                    return -1;
                }
            }
            cout<<"找到对应的session,但是sequence不相同"<<endl;
        }
    }
    cout<<"没有找到session和sequence均符合条件的会话;"<<endl;
    return -1;
};

int ActionFileACKSend(uint32_t session_id=0,uint32_t sequence=0){
    header send_packet_header{.file_proto{.file_code=FileAck,.frame_transport_length=0,.session_id=session_id,.sequence=sequence}};
    send_packet_header.file_proto.data_length = 0;
    send(conn_client_fd,&send_packet_header + send_packet_header.file_proto.data_length,HEADER_SIZE,0);
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
        if(file_ss_iter->session_id == received_packet_header->file_proto.session_id){
            close(file_ss_iter->file_fd);
            file_list->erase(file_ss_iter);
            return 0;
        }
    }
    cout<<"没有在列表里找到相应的文件"<<endl;
    return -1;
};

int ActionSendMessageToServer(const char* message){
    if(self_data->state == Online) {
        char send_message_packet[HEADER_SIZE + BUFFER_SIZE]={0};
        auto * send_message_header=(header*)send_message_packet;
        auto * send_message_data = (data *)(send_message_packet+HEADER_SIZE);

        send_message_header->msg_proto.proto = ProtoMsg;
        strcpy(send_message_data->msg_general.msg_data, message);

        cout << "[send->server] message send to server:" << send_message_data->msg_general.msg_data << endl;
        send_message_header->msg_proto.data_length = strlen(send_message_data->msg_general.msg_data)
                                                     + sizeof(send_message_data->msg_general.userName);
        send(conn_client_fd, send_message_packet, HEADER_SIZE + send_message_header->msg_proto.data_length, 0);
        cue_flag = true;
        return 0;
    }
    else{
        cout<<"[send->server] Error: please connect server..."<<endl;
        cue_flag = true;
        return -1;
    }
}

int ActionMessageFromServer(const char* receive_packet_total){
    auto* packet_header = (header*)receive_packet_total;
    auto* packet_data  = (data*)(receive_packet_total+HEADER_SIZE);
    cout<<"\n[sur-rcv] message form server("<<conn_client_fd<<"):\033[34m"<<packet_data->msg_general.msg_data<<"\033[0m"<<endl;

    // 广播到所有客户端（gui）
    for (auto & client_point : *gui_client_list) {
        cout<<"[sur-rcv] send message to client("<<client_point.nickname<<").socket_fd = "<<client_point.fd<<endl;
        packet_header->base_proto.data_length = strlen(packet_data->msg_general.msg_data)+sizeof(USER_NAME);
        send(client_point.fd, receive_packet_total, HEADER_SIZE + packet_header->base_proto.data_length, 0);
    }
    return 0;
};
