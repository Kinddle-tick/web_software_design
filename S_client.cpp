#include "My_console.h"
#include <list>

#define GUI_MSG 0
#define GUI_FILE 1
#define GUI_SET_NAME_AND_PASSWORD 2
#define GUI_CHANGE_NAME_AND_PASSWORD 3
#define DEBUG_LEVEL 0

struct fd_info{
    int fd;
    USER_NAME nickname;
};

char user_name[USERNAME_LENGTH] = "default";
unsigned int password = 12345;

// 初始化 准备接收消息

//int client_ID=0;
int conn_client_fd,gui_server_fd;
using namespace std;
int max_fd=1;
list<fd_info>* gui_client_list;
fd_set * client_fd_set;

//declare
unsigned int ActionCHAPResponse(const char*);
int ActionPrintConsoleHelp(const char*);
void Delay(int time_ms)//time单位ms
{
    clock_t now = clock();
    while(clock() - now < time_ms*CLOCKS_PER_SEC/1000);
}

ssize_t EventServerMsg() {
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventStdinMsg"<<endl;
#endif
    char receive_message[HEADER_SIZE + BUFFER_SIZE];
    bzero(receive_message, HEADER_SIZE + BUFFER_SIZE);
    ssize_t byte_num = read(conn_client_fd, receive_message, BUFFER_SIZE + HEADER_SIZE);
    if (byte_num > 0) {
        header packet_header{};
        memcpy(&packet_header, receive_message, HEADER_SIZE);
        unsigned int answer;//just for debug
        switch (packet_header.proto) {
            case PROTO_MSG:
                printf("message form server(%d):%s\n", conn_client_fd, receive_message + HEADER_SIZE);
                // 广播到所有客户端
                for (auto & client_point : *gui_client_list) {
                    printf("send message to client(%s).fd = %d\n", client_point.nickname, client_point.fd);
                    send(client_point.fd, receive_message, strlen(receive_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
                }
                break;
            case PROTO_CHAP:
                switch (packet_header.chap_proto.code) {
                    case CHAP_CODE_CHALLENGE:
                        ActionCHAPResponse(receive_message);
                        break;
                    case CHAP_CODE_SUCCESS:
                        printf("CHAP success\n");
                        break;
                    case CHAP_CODE_FAILURE:
                        printf("CHAP failure\n");
                        break;
                    default:
                        printf("Invalid CHAP_CODE\n");
                }
                break;
            case PROTO_FILE:
                break;
            default:
                printf("Invalid proto,drop the packet...");
        }

    } else if(byte_num < 0){
        printf("receive error!\n");
    }
    return byte_num;
}

int EventServerQuit(){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: EventServerQuit"<<endl;
#endif
    char recv_msg[HEADER_SIZE+BUFFER_SIZE];
    strcpy(recv_msg+HEADER_SIZE,"server exit");
    for(auto & client_point : *gui_client_list){
        printf("**server exit");
        send(client_point.fd, recv_msg+HEADER_SIZE, strlen(recv_msg+HEADER_SIZE)+1, 0);
    }
    return 0;
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
        printf("GUI(%s) joined!\n",tmp_client->nickname);
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
        /** region ## 收到GUI内容后的一系列处理手段 ## */
        printf("message form socket(%s):%s\n", client->nickname, recv_msg+HEADER_SIZE);
        send(conn_client_fd, recv_msg, strlen(recv_msg + HEADER_SIZE) + HEADER_SIZE + 1, 0);
        /** endregion */
    }
    else if(byte_num < 0){
        printf("recv error!");
    }else{
        printf("socket(%s) exit! \n and the Procedure is exiting...\n",client->nickname);
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
    /** region ## 检查<标准输入>是否存在于ser_fdset集合中（也就是说，检测到键盘输入时，做如下事情） ## */
    char input_msg[HEADER_SIZE+BUFFER_SIZE]={0};
    char* para[128]={nullptr};//顶天接受128个用空格分开的参数了
    unsigned int para_count=0;
    bool blank_flag= true;
    cin.getline(input_msg,HEADER_SIZE+BUFFER_SIZE,'\n');
    if(strcmp(input_msg,"exit")==0){
        return CLIENT_EXIT;
    }
    if(strlen(input_msg)==0){
        // 关于cin.eofbit何时置位的问题 https://en.cppreference.com/w/cpp/io/ios_base/iostate
        // cin.badbit是非常严重的错误，（比如空指针..） 区区后台运行根本不会导致
        if(cin.rdstate()&(istream::eofbit|istream::failbit)){//当读取失败或者读取到达流尾
            printf("发现处于后台工作 stdin被抑制...\n");
            return STDIN_EXIT;
        }
        return NORMAL;
    }
    //根据空格分离参数 para中将会保存每一个参数的指针 para_count则为参数的个数
    for(char * chr_ptr=input_msg;*chr_ptr != 0;chr_ptr++){
        if(*chr_ptr==' '){
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
    } else if(!strcmp(para[0],"login")){
        if(para_count>2){
            strcpy(user_name,para[1]);
            password = (unsigned int)strtoul(para[2],nullptr,10);

            return CONNECT_SERVER;
        }else{
            return CONNECT_SERVER;
        }
    }

    /** endregion */
    return NORMAL;
}
int ActionPrintConsoleHelp(const char* first_para = nullptr){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: ActionPrintConsoleHelp"<<endl;
#endif
    cout<<"[help]: ";
    if(first_para==nullptr){
        //region 可用的指令
        cout<<"可使用的指令如下:(区分大小写)\n"
              "login            连接到服务器并使用账号密码登录\n"
              "register         连接到服务器并尝试使用账号密码注册\n"
              "msg              发送一条信息到服务器，默认广播给所有人\n"
              "change_password  更改密码\n"
              "change_username  更改用户名（若数据库没有重复的用户名）\n"
              "ls               查看服务器自己文件夹的内容\n"
              "install          下载指定文件"
              "输入 help [命令符号]查看每条指令的具体使用方法\n";
        //endregion
    }
        //region 指令详解
    else if (!strcmp(first_para,"login")){
        cout<<""
              "login <username> <password>\n"
              "*登录成功与否会给予反馈\n";
    } else if (!strcmp(first_para,"register")){
        cout<<""
              "register <username> <password>\n"
              "*注册成功与否会给予反馈，注册成功送一个文件夹(划掉\n";
    }else if (!strcmp(first_para,"msg")){
        cout<<""
              "msg [-t <username>] <your message>\n";
    }else if (!strcmp(first_para,"change_password")){
        cout<<""
              "change_password <old_password> <new_password>\n"
              "";//大概会调用两次CHAP 一次验证old_password 一次用异或取出密码（这绝对算作弊吧）
    }else if (!strcmp(first_para,"change_username")){
        cout<<""
              "change_username <new username>\n"
              "*修改成功与否会给予反馈\n";//大概会调用两次CHAP 一次验证old_password 一次用异或取出密码（这绝对算作弊吧）
    }else if (!strcmp(first_para,"ls")){
        cout<<"ls 暂未实现\n";
    }else if (!strcmp(first_para,"install")){
        cout<<"install 暂未实现\n";
    }else{
        cout<<"该指令无效\n";
    }
    //endregion
    cout.flush();
    return 0;
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
    send_packet_header->chap_proto.code=CHAP_CODE_RESPONSE;
    send_packet_header->chap_proto.sequence+=1;
    strcpy(send_packet_data->chap_response.username,user_name);
    send_packet_data->chap_response.answer = htonl(answer^password);
    send(conn_client_fd,send_packet_total, HEADER_SIZE+offsetof(data,chap_response.other)+1,0);
    return answer;
};

int ActionConnectServer(bool rst){
#if DEBUG_LEVEL >0
    cout<<"[DEBUG]: ActionConnectServer"<<endl;
#endif
    sockaddr_in server_addr{};
    // 面向服务器的链接 在本程序中是作为客户端存在的
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SER_PORT);
    server_addr.sin_addr.s_addr =INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);
    //ODO 看看conn_client_fd 的初始化能不能也移动到事件中 以及能否只就用conn_client_fd==0直接来判断是否rst
    if(rst){
        close(conn_client_fd);
        FD_CLR(conn_client_fd,client_fd_set);
        conn_client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(conn_client_fd == -1){
            perror("conn_client_fd socket error");
            return 1;
        }
    } else{
        conn_client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(conn_client_fd == -1){
            perror("conn_client_fd socket error");
            return 1;
        }
    }

    if(connect(conn_client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
    {
        printf("connect server successes\n");
        //add server
        FD_SET(conn_client_fd, client_fd_set);
        if(max_fd < conn_client_fd)
        {
            max_fd = conn_client_fd;
        }
        return 0;
    } else return -1;
}

int main(int argc, const char * argv[]){

    struct sockaddr_in my_addr{AF_INET};
    struct sockaddr_in server_addr{};

    if (argc!=3) {
//        std::cout<<argc;
//        std::cout<<"Usage:"<<argv[0]<<"127.0.0.1 60000"<<endl;
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
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
//    fd_set ser_fdset;
    struct timeval ctl_time{2700,0};
    gui_client_list = new list<fd_info>;
    list<fd_info>::iterator client_iterator;
    bool select_flag = true,stdin_flag= true,cue_flag = true;
    bool is_connected = false;

//    if(connect(conn_client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
//    {
//        printf("connect server successes\n");
//    }

    /** region #<折叠代码块># select前，将 ser_fdset 初始化 */
    FD_ZERO(client_fd_set);

//        //add server
//        FD_SET(conn_client_fd, client_fd_set);
//        if(max_fd < conn_client_fd)
//        {
//            max_fd = conn_client_fd;
//        }

    //add gui
    FD_SET(0,client_fd_set);
    FD_SET(gui_server_fd, client_fd_set);
    if(max_fd < gui_server_fd)
    {
        max_fd = gui_server_fd;
    }

    /** endregion */
    while(select_flag&&stdin_flag)
    {

        //select多路复用
        if(cue_flag){
            cue_flag = false;
            cout<<"(client) shell % ";
            cout.flush();
        }

        int ret = select(max_fd + 1, client_fd_set, nullptr, nullptr, &ctl_time);

        if(ret < 0){
            perror("select failure");
            break;
        }

        else if(ret == 0){
            printf("time out!");
        }

        else{

            if(FD_ISSET(0,client_fd_set))
            {
                cue_flag = true;
                switch (EventStdinMsg()) {
                    case NORMAL:
                        break;

                    case CLIENT_EXIT:
                        select_flag= false;
                        break;

                    case STDIN_EXIT:
                        FD_CLR(0,client_fd_set);
                        stdin_flag= false;
                        break;

                    case CONNECT_SERVER:
                        //add server
                        ActionConnectServer(is_connected);
                        is_connected = true;
                        break;
                }
            }
            else if(stdin_flag){
                FD_SET(0,client_fd_set);
            }

            if(FD_ISSET(conn_client_fd, client_fd_set))
            {
                /** region ## 检查<conn_client_fd>是否存在于ser_fdset集合中,如果存在 意味着服务器发送了内容 ## */
                if(EventServerMsg() == 0){
                    FD_CLR(conn_client_fd, client_fd_set);
                    printf("server(%d) exit!\n", conn_client_fd);
                    // 广播到所有客户端
                    EventServerQuit();
                }
                /** endregion */
            }
            else{
                FD_SET(conn_client_fd, client_fd_set);
            }

            if(FD_ISSET(gui_server_fd, client_fd_set)){
                /** region ## 检查<gui_server_fd>是否存在于ser_fdset集合中，如果存在 意味着有GUI界面试图连接 ## */
                EventNewGui();
                /** endregion */
            } else{
                FD_SET(gui_server_fd, client_fd_set);
            }
        }

        /** region ## 逐个检查client列表中的套接字接口是否有信息 如果有则说明GUI发送了内容到这里等待转发 ## */
        for(client_iterator = gui_client_list->begin(); client_iterator != gui_client_list->end(); ++client_iterator){
            if(FD_ISSET(client_iterator->fd,client_fd_set)){
                if (EventGUIMsg(&*client_iterator)==0){
                    /** region ## 发现客户端连接退出后的一系列处理手段 ## */
                    FD_CLR(client_iterator->fd, client_fd_set);
                    close(client_iterator->fd);
                    gui_client_list->erase(client_iterator++);
                    client_iterator--;
                    select_flag = false;
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

    gui_client_list->clear();
    delete gui_client_list;
    close(conn_client_fd);
    close(gui_server_fd);
    return 0;
}