#include "My_console.h"
#include <list>

#define backlog 16
#define DIR_LENGTH 30 //请注意根据文件夹的名称更改此数字

struct client_session{
    int socket_fd;
    USER_NAME nickname;
    char now_path[USER_PATH_MAX_LENGTH]={0};
    State state = Offline;
    clock_t tick = 0;
};
struct user_info{
    USER_NAME user_name;
    unsigned int password;
    char root_path[USER_PATH_MAX_LENGTH];
};
struct chap_session{
    uint32_t sequence;
    uint32_t answer;
    client_session * client;
    clock_t tick = 0;
};
struct file_session{
    uint32_t session_id;
    uint32_t sequence;
    int file_fd;
    FILE * file_ptr;
    State state;
    clock_t tick;
};


using namespace std;
//int conn_client_fd;
//int client_ID = 0;
//int logger_fd= 0;
//FILE *logger_fptr = nullptr;
int conn_server_fd = 0;
unsigned int session_id = 1;
static list<user_info>* user_list;
static list<client_session>* client_list;
static list<chap_session>* chap_list;
static list<file_session>* file_list;

fd_set ser_fd_set{0};
int max_fd=1;
const char server_data_dir[DIR_LENGTH] = "server_data/";

//declare
int EventReceiveStdin();
int EventNewClient();
int EventClientMessage(client_session *client);

unsigned int ActionControlUnregistered(client_session *);
unsigned int ActionChapChallenge(client_session *client);
unsigned int ActionChapJustice(const char *receive_packet_total, client_session *client);
int ActionControlLogin(const char*, client_session*);
int ActionControlLsResponse(client_session*);
int ActionFileResponse(const char*, client_session*);
int ActionFileTranslating(file_session*, client_session*);
int ActionFileAckReceived(const char*, client_session*);
int ActionFileEndSend(const char*, client_session*);
int ActionMessageProcessing(const char *receive_packet_total, client_session *client);


int EventReceiveStdin(){
    char input_message[BUFFER_SIZE-sizeof(USER_NAME)]={0};
    bzero(input_message, BUFFER_SIZE-sizeof(USER_NAME)); // 将前n个字符串清零
    fgets(input_message, BUFFER_SIZE-sizeof(USER_NAME), stdin);
    if(strcmp(input_message,"exit\n")==0){
        return 1;
    }
    if(strlen(input_message)==0) {
        if(cin.rdstate()&(istream::eofbit|istream::failbit)){//当读取失败或者读取到达流尾
            cout<<("发现处于后台工作 stdin被抑制...")<<endl;
            return 2;
        }
        return 0;
    }

    for(auto & client_iterator : *client_list){
        printf("send message to client(%s).socket_fd = %d\n", client_iterator.nickname,client_iterator.socket_fd);
        char packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total+HEADER_SIZE);
        packet_header->proto = ProtoMsg;
        strcpy(packet_data->msg_general.userName,"");
        strcpy(packet_data->msg_general.msg_data,input_message);
        packet_header->msg_proto.data_length=sizeof(USER_NAME)+ strlen(input_message) + 1;
        send(client_iterator.socket_fd, input_message,HEADER_SIZE + packet_header->msg_proto.data_length, 0);
    }
    return 0;
}

int EventNewClient(){
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(conn_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        client_session tmp_cs = {.socket_fd = client_sock_fd,
                                 .state = Offline,.tick=clock()};
        strcpy(tmp_cs.nickname,"Unknown");
        client_list->push_back(tmp_cs);
        printf("client-Offline joined in socket_fd(%d)!\n",client_sock_fd);
        fflush(stdout);
    }
    return client_sock_fd;
}

int EventClientMessage(client_session* client){

    char receive_message[HEADER_SIZE + BUFFER_SIZE]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(client->socket_fd, receive_message, HEADER_SIZE);
    byte_num += read(client->socket_fd,receive_message+HEADER_SIZE,packet_header->base_proto.data_length);
    if(byte_num >0){
        auto* receive_packet_header = (header*) receive_message;
        switch (receive_packet_header->proto) {
            case ProtoCTL:
                switch (receive_packet_header->ctl_proto.ctl_code) {
                    case CTLLogin:
                        ActionControlLogin(receive_message,client);
                        break;
                    case CTLRegister:
                        break;
                    case CTLChangeUsername:
                        break;
                    case CTLChangePassword:
                        break;
                    case CTLLs:
                        ActionControlLsResponse(client);
                        break;
                    deault:
                        cout<<"receive invalid CTL_code";
                        break;
                }
                break;
            case ProtoMsg:
                ActionMsgProcessing(receive_message,client);
                break;
            case ProtoCHAP:
                switch (receive_packet_header->chap_proto.chap_code) {
                    case CHAPResponse:
                        ActionCHAPJustice(receive_message,client);
                        break;
                    default:
                        printf("\tInvalid CHAP_CODE\n");
                }
                break;
            case ProtoFile:
                switch (receive_packet_header->file_proto.file_code) {
                    case FileRequest:
                        ActionFileResponse(receive_message,client);
                        break;
                    case FileAck:
                        ActionFileAckReceived(receive_message,client);
                }
                break;
        }

    }
    else if(byte_num < 0){
        printf("recv error!");
    } else{//byte_num = 0
        printf("client(%s) exit!\n",client->nickname);
        return -1;
    }
    return 0;
}

int ActionMessageProcessing(const char* receive_packet_total, client_session* client){
    auto* packet_header = (header*)receive_packet_total;
    auto* packet_data = (data*)(receive_packet_total+HEADER_SIZE);
//    USER_NAME from_user = {0};
//    strcpy(from_user,packet_data->msg_general.userName);

    printf("message form client(%s):%s\n", client->nickname, packet_data->msg_general.msg_data );
    packet_header->msg_proto.data_length = sizeof(USER_NAME) + strlen(packet_data->msg_general.msg_data);
    bool flood_flag = (strcmp(packet_data->msg_general.userName,"") == 0);
    strcpy(packet_data->msg_general.userName,client->nickname);

    // 广播到所有客户端
    for(auto client_point = client_list->begin(); client_point != client_list->end(); ++client_point){
        if(flood_flag){
            if(client_point->socket_fd != client->socket_fd){
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     HEADER_SIZE + packet_header->msg_proto.data_length, 0);
            }else if(client_list->size() == 1){
                printf("echo message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     HEADER_SIZE + packet_header->msg_proto.data_length, 0);
            }
        }
        else{
            if(strcmp(client_point->nickname,packet_data->msg_general.userName) == 0){
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     HEADER_SIZE + packet_header->msg_proto.data_length, 0);
            }
        }

    }
    return 0;
}

int ActionControlLogin(const char* packet_total, client_session* client){
    auto * packet_data  = (data *)(packet_total+HEADER_SIZE);
    for(auto & user_iter : *user_list){
        if(!strcmp(user_iter.user_name,packet_data->ctl_login.userName)){
            strcpy(client->nickname, packet_data->ctl_login.userName);
            strcpy(client->now_path, user_iter.root_path);
            ActionCHAPChallenge(client);
            client->state = CHAPChallenging;
            return 0;
        }
    }
    cout<<"没找到用户名：("<<packet_data->ctl_login.userName<<")"<<endl;
    ActionControlUnregistered(client);
    client->state = Offline;
    return 1;
}

unsigned int ActionControlUnregistered(client_session* client){
    char send_packet[HEADER_SIZE+BUFFER_SIZE];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = ProtoCTL;
    send_packet_header->ctl_proto.ctl_code=CTLUnregistered;
    send_packet_header->ctl_proto.data_length =0;
    send(client->socket_fd, send_packet, HEADER_SIZE+ send_packet_header->ctl_proto.data_length, 0);
    return 0;
}

int ActionControlLsResponse(client_session* client){
    if(client->state == Online){
        DIR *dir = opendir(client->now_path);
        if(dir == nullptr){
            cout<<"client ("<<client->nickname<<")的ls请求被拒绝: 用户根目录无效"<<endl;
            return -1;
        }
        char response_packet[HEADER_SIZE+BUFFER_SIZE]= {0};
        auto * response_header = (header*)response_packet;
        auto * response_data = (data*)(response_packet+HEADER_SIZE);
        response_header->proto = ProtoCTL;
        response_header->ctl_proto.ctl_code = CTLLs;
        dirent* dir_ptr;
        while((dir_ptr = readdir(dir))!= nullptr){
            strcat(response_data->ctl_ls.chr,dir_ptr->d_name);
            strcat(response_data->ctl_ls.chr,"\n");
        }
        closedir(dir);
        response_header->ctl_proto.data_length =  strlen(response_data->ctl_ls.chr);
        send(client->socket_fd, response_packet, HEADER_SIZE +response_header->ctl_proto.data_length, 0);
        return 0;
    } else{
        cout<<"client ("<<client->nickname<<")的ls请求被拒绝: 非Online状态没有权限"<<endl;
        return -1;
    }
}

unsigned int ActionChapChallenge(client_session* client){
    char packet_total[HEADER_SIZE + BUFFER_SIZE]={};
    clock_t seed = clock();
    srand(seed);//这里用time是最经典的 但是因为服务端总是要等很久 每次连接的时间其实差别很大 所以用clock()我觉得不会有问题
    uint8_t  one_data_size = rand() % 3; // 2**one_data_size arc4random()是一个非常不错的替代解法 但是这边还是用了rand先
    uint32_t number_count = 10 + rand() % (BUFFER_SIZE / (1 << one_data_size ) - 10);//至少要有10个数据 （10～40字节）

    auto sequence = (uint32_t)(rand() % ((0U - 1)/4 - chap_list->size()));
    auto chap_iterator = chap_list->begin();
    if(!chap_list->empty()){
        for(chap_iterator = chap_list->begin();chap_iterator!=chap_list->end();++chap_iterator){
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

    auto* packet_header=(header *)packet_total;
    packet_header->chap_proto.proto = ProtoCHAP;
    packet_header->chap_proto.chap_code = CHAPChallenging;
    packet_header->chap_proto.one_data_size=one_data_size;
    packet_header->chap_proto.number_count=number_count;
    packet_header->chap_proto.sequence = sequence;
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
        num tmpnum{.int32=rand()};
        tmpnum.uint16 = one_data_size==1 ? htons(tmpnum.uint16):tmpnum.uint16;
        tmpnum.uint32 = one_data_size==2 ? htonl(tmpnum.uint32):tmpnum.uint32;
        /**
         * 上文与之等价
            switch (one_data_size) {
            case 0:{
                break;
            }
            case 1:{
                tmpnum.uint16 = htons(tmpnum.uint16);
                break;
            }
            case 2:{
                tmpnum.uint32 = htonl(tmpnum.uint32);
                break;
            }
        }
         * */

        memcpy(&(packet_total[HEADER_SIZE + i * (1 << one_data_size)]), &tmpnum, 1<<one_data_size);
        answer += one_data_size==0 ? tmpnum.uint8:0;
        answer += one_data_size==1 ? ntohs(tmpnum.uint16):0;
        answer += one_data_size==2 ? ntohl(tmpnum.uint32):0;
    }
    packet_header->chap_proto.data_length = number_count * (1 << one_data_size);
    send(client->socket_fd, packet_total, HEADER_SIZE + packet_header->chap_proto.data_length, 0);
    chap_list->insert(chap_iterator, chap_session{.sequence=sequence,.answer=answer,.client = client,.tick=clock()});
    return answer;
}

unsigned int ActionChapJustice(const char* receive_packet_total, client_session* client){
    auto* receive_packet_header = (header *)(receive_packet_total);
    auto* receive_packet_data = (data *)(receive_packet_total+HEADER_SIZE);
    char* send_packet_total[HEADER_SIZE];
    auto* send_packet_header = (header*)send_packet_total;
    memcpy(send_packet_header,receive_packet_total,HEADER_SIZE);
    send_packet_header->chap_proto.sequence+=1;
    for (auto chap_iter=chap_list->begin();chap_iter!= chap_list->end();++chap_iter) {
        if(chap_iter->sequence == receive_packet_header->chap_proto.sequence - 1){
            for(auto & user_iter : *user_list){
                if(!strcmp(user_iter.user_name,receive_packet_data->chap_response.userName)&&
                !strcmp(user_iter.user_name,chap_iter->client->nickname)){
                    if (ntohl(receive_packet_data->chap_response.answer)==(chap_iter->answer^user_iter.password)){
                        send_packet_header->chap_proto.chap_code=CHAPSuccess;
                        strcpy(client->nickname,receive_packet_data->chap_response.userName);
                        send_packet_header->chap_proto.data_length = 0;
                        send(client->socket_fd, send_packet_total,
                             HEADER_SIZE + send_packet_header->chap_proto.data_length, 0);
                        printf("\tCHAP_SUCCESS,\"%s\" login in socket_fd(%d)\n",client->nickname,client->socket_fd);
                        client->state=Online;
                    } else{
                        send_packet_header->chap_proto.chap_code=CHAPFailure;
                        send_packet_header->chap_proto.data_length = 0;
                        send(client->socket_fd, send_packet_total,
                             HEADER_SIZE + send_packet_header->chap_proto.data_length, 0);
                        printf("\tCHAP_FAILURE in socket_fd(%d)\n",client->socket_fd);
                        client->state=Offline;
                    }
                    printf("\t清除CHAP_challenge记录：sequence:%u answer:%u\n",chap_iter->sequence,chap_iter->answer);
                    chap_list->erase(chap_iter);
                    return 0;
                }
            }
            printf("\tCHAP_ERROR:未搜寻到CHAP请求的用户名\n");
            client->state=Offline;
            return 2;
        }
    }
    printf("\tCHAP_ERROR:未找到对应的CHAP_challenge记录\n");
    client->state=Offline;
    return 1;
}

int ActionFileResponse(const char* receive_packet_total, client_session* client){
    auto* receive_packet_header = (header *)(receive_packet_total);
    auto* receive_packet_data = (data *)(receive_packet_total+HEADER_SIZE);
    cout<<"请求文件名:"<<receive_packet_data->file_request.file_path<<endl;
    cout<<"当前目录:"<<client->now_path<<endl;

    char request_path[USER_PATH_MAX_LENGTH]={0};
    strcpy(request_path,client->now_path);
    strcat(request_path,"/");
    strcat(request_path,receive_packet_data->file_request.file_path);
    if(access(request_path,R_OK)==0){
        //获取文件指针 约定会话号 约定初始sequence号 构造全局的session使得后续收到报文时持续跟踪之（主键是会话号）
        int fd_tmp = open(request_path,O_RDONLY);
        if(fd_tmp<0){
            perror("文件打开失败");
            return -1;
        }

        file_session tmp_session{};
        char send_packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total+HEADER_SIZE);
        tmp_session.state = FileInit;
        tmp_session.tick = clock();
        tmp_session.file_fd = fd_tmp;
        tmp_session.file_ptr = fdopen(fd_tmp,"rb");
        srand(clock());
        send_packet_header->file_proto.sequence = tmp_session.sequence = rand();
        send_packet_header->file_proto.session_id = tmp_session.session_id = session_id++;
        file_list->push_back(tmp_session);
        strcpy(send_packet_data->file_response.file_path,receive_packet_data->file_request.file_path);
        send_packet_data->file_response.init_clock = tmp_session.tick;
        send_packet_header->proto = ProtoFile;
        send_packet_header->file_proto.file_code=FileResponse;
        send_packet_header->file_proto.data_length =  strlen(send_packet_data->file_response.file_path)
                +sizeof(send_packet_data->file_response.init_clock) ;
        send(client->socket_fd, send_packet_total,HEADER_SIZE+send_packet_header->file_proto.data_length, 0);

        cout<<"试图发送文件:"<<request_path<<endl;
        return 0;
    }
    else{
        cout<<"该路径文件不存在或文件不可用:"<<request_path<<endl;
        return -1;
    }
};

int ActionFileTranslating(file_session* file_ss , client_session* client){
    char send_packet_total[HEADER_SIZE+BUFFER_SIZE]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total+HEADER_SIZE);
    send_packet_header->file_proto.frame_transport_length= read(file_ss->file_fd,
                                                                send_packet_data->file_transport.file_data,
                                                                sizeof(send_packet_data->file_transport.file_data));
    send_packet_header->proto=ProtoFile;
    send_packet_header->file_proto.file_code=FileTransporting;
    send_packet_header->file_proto.session_id = file_ss->session_id;
    send_packet_header->file_proto.sequence = file_ss->sequence;
    send_packet_data->file_transport.CRC_32 = 0; //先不写

    file_ss->sequence +=  send_packet_header->file_proto.frame_transport_length;
    file_ss->state = FileTransporting;
    if(send_packet_header->file_proto.frame_transport_length == 0){
        cout<<"文件传输完毕"<<endl;
        ActionFileEndSend(send_packet_total,client);
        return 0;
    }else if(send_packet_header->file_proto.frame_transport_length < 0){
        cout<<"文件读取失败"<<endl;
        return -1;
    }else{
        send_packet_header->file_proto.data_length = sizeof(send_packet_data->file_transport.CRC_32)
                +send_packet_header->file_proto.frame_transport_length;
        send(client->socket_fd, send_packet_total,
             HEADER_SIZE + send_packet_header->file_proto.data_length , 0);
//        fprintf(logger_fptr,"fd:%ld完成了一轮发送",clock()-file_ss->tick);
        return 0;
    }
}
int ActionFileAckReceived(const char* received_packet_total, client_session* client){
    auto* received_packet_header = (header*)received_packet_total;
    for(auto &file_ss_iter:*file_list){
        if(file_ss_iter.session_id == received_packet_header->file_proto.session_id){
            if(file_ss_iter.sequence == received_packet_header->file_proto.sequence){
                if(file_ss_iter.state==FileInit){
                    cout<<"client "<<client->nickname<<"准备好接受文件传输！"<<endl;
                    ActionFileTranslating(&file_ss_iter,client);
                    return 0;
                }
                if(file_ss_iter.state == FileTransporting){
                    ActionFileTranslating(&file_ss_iter,client);
                    return 0;
                }
            }
        }
    }
    cout<<"invalid File Ack packet"<<endl;
    return -1;
};

int ActionFileEndSend(const char* raw_packet,client_session* client){
    auto * send_packet_header = (header*)raw_packet;
    send_packet_header->file_proto.file_code = FileEnd;
    send_packet_header->file_proto.data_length = 0;
    send(client->socket_fd,raw_packet,HEADER_SIZE+send_packet_header->file_proto.data_length,0);
    for(auto file_ss_iter = file_list->begin(); file_ss_iter!=file_list->end();++file_ss_iter){
        if(file_ss_iter->session_id == send_packet_header->file_proto.session_id){
            close(file_ss_iter->file_fd);
            file_ss_iter->state = FileEnd;
            file_list->erase(file_ss_iter);
            cout<<"文件指针已处理";
            return 0;
        }
    }

    cout<<"没有在列表里找到相应的文件"<<endl;
    return -1;
}

clock_t time_ms(clock_t a,clock_t b){
    if(a-b >0){
        return (a-b)/CLOCKS_PER_SEC;
    }else{
        return (b-a)/CLOCKS_PER_SEC;
    }
}
//int logger(int fd,const char * str_write,int data){
//    if(fd>0){
//        char tmp[];
//
//        return write(fd,str_write,strlen(str_write)+1);
//    }
//    return -1;
//}
int main(int agrc,char **argv){
    char main_general_buffer[1024]={0};
    user_list = new list<user_info>;
    chap_list = new list<chap_session>;
    file_list = new list<file_session>;
    client_list = new list<client_session>;

    mkdir(server_data_dir,S_IRWXU);

    //region 读取文件夹下user_sheet.csv 初始化用户列表
    char user_sheet_path[DIR_LENGTH+20];
    char user_root_path[USER_PATH_MAX_LENGTH];

    strcpy(user_sheet_path,server_data_dir);
    strcat(user_sheet_path,"user_sheet.csv");
    ifstream user_sheet_istream(user_sheet_path);
    string line;
    user_info tmp_user{};
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
        if(strlen(line.c_str())>USERNAME_LENGTH){
            printf("name:'%s' is too long,drop it",line.c_str());
        }
        strcpy(tmp_user.user_name,line.c_str());
        strcpy(user_root_path,server_data_dir);
        strcat(user_root_path,tmp_user.user_name);
        int debug_tmp = access(user_root_path,R_OK|W_OK|F_OK);
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
        user_list->push_back(tmp_user);
    }

    if(user_list->empty()){
        printf("Warning: using default user_list...");
        user_info tmp_ui = {.password=12345};
        strcpy(tmp_ui.user_name,"default");
        user_list->push_back(tmp_ui);
    }
    user_sheet_istream.close();
    //endregion

    //region 一些老生常谈的socket的初始化操作
    struct sockaddr_in ser_addr{};
    ser_addr.sin_family= AF_INET;    //IPV4
    ser_addr.sin_port = htons(SER_PORT);
    ser_addr.sin_addr.s_addr = INADDR_ANY;  //指定的是所有地址

    //creat server socket 准备server的本体套接字
    if((conn_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("creat failure");
        return -1;
    }

    //bind socket  绑定
    if(::bind(conn_server_fd, (const struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        perror("bind failure");
        return -1;
    }

    //listen 监听
    if(listen(conn_server_fd, backlog) < 0)
    {
        perror("listen failure");
        return -1;
    }
    //endregion

    //region fd_set的初始化操作等
    //fd_set  准备fd_set
    bool select_flag = true,stdin_flag= true;
    struct timeval ctl_time{2700, 0};
//    logger_fd = open("server_logger",O_TRUNC|O_WRONLY|O_CREAT,S_IXUSR|S_IWUSR|S_IRUSR);
//    logger_fptr = fdopen(logger_fd,"w");
    //add standard input
    FD_SET(0,&ser_fd_set);
    //add server
    FD_SET(conn_server_fd, &ser_fd_set);
    if(max_fd < conn_server_fd)
    {
        max_fd = conn_server_fd;
    }
    //endregion

    printf("Waiting for connection!\n");
    while(select_flag) {
        int ret = select(max_fd + 1, &ser_fd_set, nullptr, nullptr, &ctl_time);

        if (ret > 0) {
            if (FD_ISSET(0, &ser_fd_set)) //<标准输入>是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                switch (EventRcvStdin()) {
                    case 0:
                        break;

                    case 1: {
                        select_flag = false;
                    }
                    case 2: {
                        FD_CLR(0, &ser_fd_set);
                        stdin_flag = false;
                        break;
                    }
                }
            } else if (stdin_flag) {
                FD_SET(0, &ser_fd_set);
            }

            if (FD_ISSET(conn_server_fd, &ser_fd_set))//<server>是否存在于ser_fdset集合中,如果存在 意味着有connect请求
            {
                EventNewClient();
            } else {
                FD_SET(conn_server_fd, &ser_fd_set);
            }

            for (auto client_iterator = client_list->begin(); client_iterator != client_list->end(); ++client_iterator) {
                if (FD_ISSET(client_iterator->socket_fd, &ser_fd_set)) {
                    client_iterator->tick =clock();
                    if (EventClientMsg(&*client_iterator) == -1) {
                        FD_CLR(client_iterator->socket_fd, &ser_fd_set);
                        close(client_iterator->socket_fd);
                        client_list->erase(client_iterator++);
                        client_iterator--;
                    }
                } else {
                    FD_SET(client_iterator->socket_fd, &ser_fd_set);
                    if (max_fd < client_iterator->socket_fd) {
                        max_fd = client_iterator->socket_fd;
                    }
                }
            }

        }

        //deal with the message
        else if (ret == 0) {
            printf("time out!");
            if(stdin_flag){
                FD_SET(0,&ser_fd_set);
            }
            FD_SET(conn_server_fd, &ser_fd_set);
            for(auto client_iter:*client_list){
                FD_SET(client_iter.socket_fd, &ser_fd_set);
            }

        } else {
            perror("select failure\n");
            continue;
        }
    }

    printf("exiting...\n");
//    for(auto client_iter : *client_list){
//        close(client_iter.socket_fd);
//    }
    close(conn_server_fd);
    client_list->clear();
//    close(logger_fd);
    //region 存储用户数据
    int user_sheet_fd = open(user_sheet_path,O_RDWR|O_TRUNC);
    if(user_sheet_fd<0){
        perror("open failed");
    }
    for(auto & user_iter : *user_list){
        sprintf(main_general_buffer,"%s,%d\n",user_iter.user_name,user_iter.password);
        write(user_sheet_fd,main_general_buffer, strlen(main_general_buffer));
        printf("write-> %s",main_general_buffer);
    }
    close(user_sheet_fd);
    //endregion

    delete client_list;
    delete chap_list;
    delete user_list;
    return 0;
}


