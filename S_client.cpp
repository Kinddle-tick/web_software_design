#include "My_console.h"
#include <list>

//#define GUI_MSG 0
//#define GUI_FILE 1
//#define GUI_SET_NAME_AND_PASSWORD 2
//#define GUI_CHANGE_NAME_AND_PASSWORD 3
#define DEBUG_LEVEL 0
#define DIR_LENGTH 30 //请注意根据文件夹的名称更改此数字

struct file_session{
    uint32_t session_id;
    uint32_t sequence;
    int file_fd;
};

struct fd_info{
    int fd;
    USER_NAME nickname;
};

struct client_self_data{
    USER_NAME confirmUserName = "";
    USER_NAME tmpUserName = "";
    uint32_t password = 0;
    State state = Offline;
    const char client_data_root[DIR_LENGTH] = "client_data/";
    char client_data_dir_now[DIR_LENGTH] = "client_data/";
}*self_data;

// 初始化 准备接收消息

//int client_ID=0;
int conn_client_fd,gui_server_fd;
using namespace std;
int max_fd=1;
bool cue_flag = true;
list<fd_info>* gui_client_list;
list<file_session>* file_list;
fd_set * client_fd_set;
const char* client_data_dir = "client_data/";
//const char client_data_dir[DIR_LENGTH] = "client_data/";
//declare
void BaseActionInterrupt();
unsigned int ActionCHAPResponse(const char*);
int ActionPrintConsoleHelp(const char*);
int ActionConnectServer();
int ActionControlLogin();
int ActionSendMessageToServer(const char*);
int ActionControlLs();
int ActionFileRequest(const char*);
int ActionFileResponseReceived(const char * );
int ActionFileTransportingReceived(const char*);
int ActionFileACKSend(uint32_t, uint32_t);
int ActionFileErrorSend(uint32_t, uint32_t);
int ActionFileEndReceived(const char*);
//region delay
//void Delay(int time_ms)//time单位ms
//{
//    clock_t now = clock();
//    while(clock() - now < time_ms*CLOCKS_PER_SEC/1000);
//}
//endregion

ssize_t EventServerMsg() {
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
#define SERVER_SHUTDOWN 0
    char receive_message[HEADER_SIZE + BUFFER_SIZE];
    bzero(receive_message, HEADER_SIZE + BUFFER_SIZE);
    ssize_t byte_num = read(conn_client_fd, receive_message, BUFFER_SIZE + HEADER_SIZE);
    if (byte_num > 0) {
        auto* packet_header = (header *)(receive_message);

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

                }
                break;
            case ProtoMsg:
                BaseActionInterrupt();
                cout<<"\n[sur-rcv] message form server("<<conn_client_fd<<"):\033[34m"<<receive_message + HEADER_SIZE<<"\033[0m"<<endl;
                // 广播到所有客户端（gui）
                for (auto & client_point : *gui_client_list) {
                    cout<<"[sur-rcv] send message to client("<<client_point.nickname<<").socket_fd = "<<client_point.fd<<endl;
                    send(client_point.fd, receive_message, strlen(receive_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
                }
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
                cout<<("[sur-rcv] Invalid proto,drop the packet...");
                cue_flag = true;
        }
        //endregion

    }
    else if(byte_num < 0){
        cout<<("[sur-rcv] receive error!\n");
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
            send(client_point.fd, receive_message+HEADER_SIZE, strlen(receive_message+HEADER_SIZE)+1, 0);
        }
    }
    return SERVER_SHUTDOWN;
}

//int EventServerQuit(){
//#if DEBUG_LEVEL >0
//    cout<<"[DEBUG]: EventServerQuit"<<endl;
//#endif
//    char recv_msg[HEADER_SIZE+BUFFER_SIZE];
//    strcpy(recv_msg+HEADER_SIZE,"server exit");
//    for(auto & client_point : *gui_client_list){
//        cout<<("**server exit");
//        send(client_point.socket_fd, recv_msg+HEADER_SIZE, strlen(recv_msg+HEADER_SIZE)+1, 0);
//    }
//    return 0;
//}

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
#define CLIENT_EXIT (-1)
#define STDIN_EXIT (-2)
#define NORMAL 0
#define CONNECT_SERVER 1
#define OTHER_ERROR (-3)
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
        return CLIENT_EXIT;
    }
    if(strlen(input_msg)==0){
        // 关于cin.eofbit何时置位的问题 https://en.cppreference.com/w/cpp/io/ios_base/iostate
        // cin.badbit是非常严重的错误，（比如空指针..） 区区后台运行根本不会导致
        if(cin.rdstate()&(istream::eofbit|istream::failbit)){//当读取失败或者读取到达流尾
            cout<<("发现处于后台工作 stdin被抑制...")<<endl;
            return STDIN_EXIT;
        }
        return NORMAL;
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
        return CONNECT_SERVER;
    }
    else if (!strcmp(para[0],"register")){
        cout<<"register 未实现";
    }
    else if (!strcmp(para[0],"msg")){
        int rtn = ActionSendMessageToServer(para[1]);
        if(rtn == -1){
            return OTHER_ERROR;
        }else{
            return NORMAL;
        }
    }
    else if (!strcmp(para[0],"logout")){
        if(conn_client_fd>0){
            close(conn_client_fd);
            FD_CLR(conn_client_fd,client_fd_set);
            conn_client_fd=0;
            self_data->state = Offline;
        }
        return NORMAL;
    }
    else if (!strcmp(para[0],"change_password")){
        cout<<"change_password";
    }
    else if (!strcmp(para[0],"change_username")){
        cout<<"change_username";
    }
    else if (!strcmp(para[0],"ls")){
        ActionControlLs();
        return NORMAL;
    }
    else if (!strcmp(para[0],"cd")){
        cout<<"cd";
    }
    else if (!strcmp(para[0],"download")){
        if(para_count < 2){
            cout<<"请输入文件名..."<<endl;
            return NORMAL;
        }
        ActionFileRequest(para[1]);
        return NORMAL;
    }
    else if (!strcmp(para[0],"upload")){
        cout<<"upload";
    }
    else{
        cout<<"Invalid command:,'"<<para[0]<<R"('try to use 'help')"<<endl;
        return NORMAL;
    }
    cout<<"这个命令也许还没有完成捏!"<<endl;
    return NORMAL;
    /** endregion */
}

void BaseActionInterrupt(){
    cout<<"\033[31m<interrupt⬇>\033[0m";
}

//region 编辑help_doc
struct help_doc{
    const char * cmd_name = nullptr;
    const char * cmd_description = nullptr;
    const char * cmd_pattern = nullptr;
}client_help[]={
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

        {},//结尾的判据
};
//endregion

int ActionPrintConsoleHelp(const char* first_para = nullptr){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: ActionPrintConsoleHelp"<<endl;
#endif
    cout<<"[help]: ";
    if(first_para==nullptr){
        //region 可用的指令
        cout<<"可使用的指令如下:(区分大小写)\n";
        cout<<sizeof(client_help);
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

unsigned int ActionCHAPResponse(const char* packet_total){
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
    for(int i = 0;i<packet_header->chap_proto.data_length;i++){
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
    send(conn_client_fd,send_packet_total, HEADER_SIZE+sizeof(send_packet_data->chap_response),0);
    return answer;
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
        send(conn_client_fd,packet_total,HEADER_SIZE+sizeof(packet_data->ctl_login)+1,0);
        self_data->state = LoginTry;
        return 0;
    } else{
        cout<< "can not send <login> packet to server in state <Offline>" <<endl;
        return 1;
    }

}

int ActionSendMessageToServer(const char* message){
    if(self_data->state == Online) {
        char send_message_body[HEADER_SIZE + BUFFER_SIZE]={0};
        auto * send_message_header=(header*)send_message_body;

        send_message_header->msg_proto.proto = ProtoMsg;
        strcpy(send_message_body+HEADER_SIZE,message);

        cout<<"[send->server] message send to server:"<<send_message_body+HEADER_SIZE<<endl;
        send(conn_client_fd, send_message_body, strlen(send_message_body + HEADER_SIZE) + HEADER_SIZE + 1, 0);
        cue_flag = true;
        return 0;
    }
    else{
        cout<<"[send->server] Error: please connect server..."<<endl;
        cue_flag = true;
        return -1;
    }
}

int ActionControlLs(){
    if(self_data->state == Online){
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto=ProtoCTL;
        packet_header->ctl_proto.ctl_code = CTLLs;
        send(conn_client_fd,packet_total,HEADER_SIZE,0);
        return 0;
    }
    else{
        cout<< "can only send ls to server in state <Online>" <<endl;
        return -1;
    }
}

int ActionFileRequest(const char* file_path){
    if(self_data->state == Online){
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto=ProtoFile;
        packet_header->file_proto.file_code = FileRequest;

        strcpy(packet_data->file_request.file_path,file_path);

        send(conn_client_fd,packet_total,HEADER_SIZE+ strlen(file_path)+1,0);
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
                                 .file_fd = tmp_fd};
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
                                     received_packet_header->file_proto.data_length);
                if(size >0){
                    ActionFileACKSend(received_packet_header->file_proto.session_id,
                                      received_packet_header->file_proto.sequence+size);
                    file_ss_iter.sequence+=size;
                    return 0;
                }
                else{
                    ActionFileErrorSend(received_packet_header->file_proto.session_id, received_packet_header->file_proto.sequence);
                    return -1;
                }
            }
        }
    }
    cout<<"没有找到session和sequence均符合条件的会话;"<<endl;
    return -1;
};

int ActionFileACKSend(uint32_t session_id=0,uint32_t sequence=0){
    header send_packet_header{.file_proto{.file_code=FileAck,.data_length=0,.session_id=session_id,.sequence=sequence}};
    send(conn_client_fd,&send_packet_header,HEADER_SIZE,0);
    return 0;
}

int ActionFileErrorSend(uint32_t session_id=0,uint32_t sequence_id=0){
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
//    for(auto &file_ss_iter:* file_list){
//        if(file_ss_iter.session_id == received_packet_header->file_proto.session_id){
//            if(file_ss_iter.sequence == received_packet_header->file_proto.sequence){
//                cout<<"找到该文件并释放fd成功"<<endl;
//                close(file_ss_iter.file_fd);
//                return 0;
//            }
//        }
//    }
//    cout<<"没有找到session和sequence均符合条件的会话;"<<endl;
    return -1;
};


int main(int argc, const char * argv[]){

    struct sockaddr_in my_addr{AF_INET};
    struct sockaddr_in server_addr{};
    mkdir(client_data_dir,S_IRWXU);
    if (argc!=3) {
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
        srand(clock());
        my_addr.sin_port = htons(rand()%30000+20000);
        bzero(&(my_addr.sin_zero), 8);
//        cout<<"using default\n";
    }
    else{
        // C++程序本体维护的socket地址 从argv中获取
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(strtol(argv[2], nullptr, 10));
        my_addr.sin_addr.s_addr=inet_addr(argv[1]);
        bzero(&(my_addr.sin_zero), 8);
//        cout<<"using para from python\n";
    }

    //region gui_server 用来准备建立与gui的链接 需要提供一个自身的地址(my_addr)
    //creat gui_server_fd socket 准备作为gui程序server的本体套接字
    if((gui_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        perror("creat gui_server_fd failure");
        return -1;
    }
    //gui_server_fd bind socket  绑定
    if(::bind(gui_server_fd, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0){
        perror("bind gui failure");
        return -1;
    }
    //gui_server_fd listen 监听
    if(listen(gui_server_fd, 10) < 0){
        perror("listen failure");
        return -1;
    }
    //endregion

    // 面向服务器的链接 在本程序中是作为客户端存在的


    // 准备select相关内容
    client_fd_set = new fd_set;
    gui_client_list = new list<fd_info>;
    file_list = new list<file_session>;
    self_data = new client_self_data{.confirmUserName ="default",.password = 12345,.state = Offline};
    strcat(self_data->client_data_dir_now,self_data->confirmUserName);
    bool select_flag = true,stdin_flag= true;
    struct timeval ctl_time{2700,0};

    /** region #<折叠代码块># select前，将 ser_fdset 初始化 */
    FD_ZERO(client_fd_set);

    //add gui_server
    FD_SET(0,client_fd_set);
    FD_SET(gui_server_fd, client_fd_set);
    if(max_fd < gui_server_fd)
    {
        max_fd = gui_server_fd;
    }

    /** endregion */

    while(select_flag)
    {
        //select多路复用
        if(cue_flag){
            cue_flag = false;
            cout<<"("<<State_description[self_data->state]<<") shell \033[35m"<<self_data->confirmUserName<<"\033[0m % ";
            cout.flush();
        }
        int ret = select(max_fd + 1, client_fd_set, nullptr, nullptr, &ctl_time);

        if(ret > 0){
            if(FD_ISSET(0,client_fd_set))
            {
                switch (EventStdinMsg()) {
                    case NORMAL:
                        cue_flag = true;
                        break;

                    case CLIENT_EXIT:
                        select_flag= false; // stdin输入指令使其退出
                        break;

                    case STDIN_EXIT:
                        FD_CLR(0,client_fd_set);
                        stdin_flag= false;
                        break;

                    case CONNECT_SERVER:
                        break;
                }
            }
            else if(stdin_flag){
                FD_SET(0,client_fd_set);
            }
            if(conn_client_fd > 0){
                if(FD_ISSET(conn_client_fd, client_fd_set))
                {
                    /** region ## 检查<conn_client_fd>是否存在于ser_fdset集合中,如果存在 意味着服务器发送了内容 ## */
//                    EventServerMsg();
                    switch (EventServerMsg()) {
                        case SERVER_SHUTDOWN:
                            select_flag = false;
                            break;
                        default:
                            break;
                    }
                    /** endregion */
                }
                else{
                    FD_SET(conn_client_fd, client_fd_set);
                }
            }
            if(gui_server_fd > 0){
                if(FD_ISSET(gui_server_fd, client_fd_set)){
                    /** region ## 检查<gui_server_fd>是否存在于ser_fdset集合中，如果存在 意味着有GUI界面试图连接 ## */
                    EventNewGui();
                    /** endregion */
                }
                else{
                    FD_SET(gui_server_fd, client_fd_set);
                }
            }
            /** region ## 逐个检查client列表中的套接字接口是否有信息 如果有则说明GUI发送了内容到这里等待转发 ## */
            for(auto client_iterator = gui_client_list->begin(); client_iterator != gui_client_list->end(); ++client_iterator){
                if(FD_ISSET(client_iterator->fd,client_fd_set)){
                    if (EventGUIMsg(&*client_iterator)==0){
                        /** region ## 发现客户端连接退出后的一系列处理手段 ## */
                        gui_client_list->erase(client_iterator++);
                        client_iterator--;
                        select_flag = false; //GUI 主动退出
                        /** endregion */
                    }
                }
                else{
                    FD_SET(client_iterator->fd,client_fd_set);
                    if(max_fd < client_iterator->fd){
                        max_fd = client_iterator->fd;
                    }
                }
            }
            /** endregion */
        }

        else if(ret == 0){
            cout<<"time out!"<<endl;
            if(stdin_flag){
                FD_SET(0,client_fd_set);
            }
            if(conn_client_fd > 0){
                FD_SET(conn_client_fd,client_fd_set);
            }
            if(gui_server_fd > 0){
                FD_SET(gui_server_fd, client_fd_set);
            }
            for (auto client_iter:*gui_client_list){
                FD_SET(client_iter.fd,client_fd_set);
            }
        }

        else{
            perror("select failure");
            break;
        }
    }

    gui_client_list->clear();
    delete gui_client_list;
    if(conn_client_fd)
        close(conn_client_fd);
    if(gui_server_fd)
        close(gui_server_fd);
    delete self_data;
    return 0;
}