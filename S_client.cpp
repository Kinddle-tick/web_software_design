#include "My_console.h"
#include <list>

struct fd_info{
    int fd;
    char nickname[21];
};

const char * my_addr_Decimal = "127.0.0.1";
const int my_port = 6000;


// 初始化 准备接收消息

int client_ID=0;
int conn_client_fd,gui_server_fd;
using namespace std;
list<fd_info>* gui_client_list;

//declare
unsigned int ActionCHAPReply(char*);

ssize_t EventServerMsg() {
    char receive_message[HEADER_SIZE + BUFFER_SIZE];
    bzero(receive_message, HEADER_SIZE + BUFFER_SIZE);
    ssize_t byte_num = read(conn_client_fd, receive_message, BUFFER_SIZE + HEADER_SIZE);
    if (byte_num > 0) {
        header packet_header{};
        memcpy(&packet_header, receive_message, HEADER_SIZE);
        unsigned int answer;
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
                answer=ActionCHAPReply(receive_message);
                printf("CHAP answer is %u\n",answer);
                sprintf(receive_message+HEADER_SIZE,"the CHAP answer is %d %c",answer,0);
                for(auto & client_point : *gui_client_list){

                    send(client_point.fd, receive_message+HEADER_SIZE, strlen(receive_message+HEADER_SIZE)+1, 0);
                }
                break;
            case PROTO_FILE:
                break;
            default:
                printf("Unknown proto,drop the packet...");
        }

    } else if(byte_num < 0){
        printf("receive error!\n");
    }
    return byte_num;
}

int EventServerQuit(){
    char recv_msg[HEADER_SIZE+BUFFER_SIZE];
    strcpy(recv_msg+HEADER_SIZE,"server exit");
    for(auto & client_point : *gui_client_list){
        printf("**server exit");
        send(client_point.fd, recv_msg+HEADER_SIZE, strlen(recv_msg+HEADER_SIZE)+1, 0);
    }
    return 0;
}

int EventNewGui(){
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(gui_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        char nickname_tmp[21]{0};
        snprintf(nickname_tmp,20,"GUI[%d]",client_ID++);
        auto * tmp_client = new fd_info{client_sock_fd};
        strcpy(tmp_client->nickname,nickname_tmp);
        gui_client_list->push_back(*tmp_client);
        printf("GUI(%s) joined!\n",tmp_client->nickname);
    }
    return 0;
}

ssize_t EventGUIMsg(list<fd_info>::iterator client){
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
    /** region ## 检查<标准输入>是否存在于ser_fdset集合中（也就是说，检测到键盘输入时，做如下事情） ## */
    char input_msg[HEADER_SIZE+BUFFER_SIZE];
    bzero(input_msg,HEADER_SIZE+BUFFER_SIZE); // 将前n个字符串清零
    fgets(input_msg+HEADER_SIZE,BUFFER_SIZE,stdin);
    if(strcmp(input_msg,"exit\n")==0){
        return -1;
    }
    if(strlen(input_msg)==0){
        printf("发现处于后台工作 stdin被抑制...\n");
        return -2;
    } else{
        printf("send message to server:%s\n",  input_msg+HEADER_SIZE);
        send(conn_client_fd, input_msg, strlen(input_msg + HEADER_SIZE) + 1 + HEADER_SIZE, 0);
        return 0;
    }
    /** endregion */
}

unsigned int ActionCHAPReply(char* packet_total){
    header packet_header;
    memcpy(&packet_header,packet_total,HEADER_SIZE);
    int one_data_size = packet_header.chap_proto.one_data_size;
    printf("one data size:%d\n",one_data_size);
    uint answer = 0;
    union num{
        int32_t int32;
        uint32_t uint32;
        uint16_t uint16;
        uint8_t uint8;
    };
    for(int i = 0;i<packet_header.chap_proto.data_length;i++){
        num tmpnum;
        memcpy(&tmpnum,packet_total+HEADER_SIZE+i*(1<<one_data_size),1<<one_data_size);
        answer += one_data_size==0 ? tmpnum.uint8:0;
        answer += one_data_size==1 ? ntohs(tmpnum.uint16):0;
        answer += one_data_size==2 ? ntohl(tmpnum.uint32):0;
        printf("%u\n",answer);
    }

    if(one_data_size==2){
        return answer;
    } else{
        return answer % (1<<(8*(1<<one_data_size)));
    }
};

int main(int argc, const char * argv[]){

    struct sockaddr_in my_addr{AF_INET};
    struct sockaddr_in server_addr{};

    if (argc!=3) {
        std::cout<<argc;
        std::cout<<"Usage:"<<argv[0]<<"127.0.0.1 60000";
        return 1;
    }
    else{
        // C++程序本体维护的socket地址 从argv中获取
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(strtol(argv[2], nullptr, 10));
        my_addr.sin_addr.s_addr=inet_addr(argv[1]);
        bzero(&(my_addr.sin_zero), 8);

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

        // 面向服务器的链接 在本程序中是作为客户端存在的
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SER_PORT);
        server_addr.sin_addr.s_addr =INADDR_ANY;
        bzero(&(server_addr.sin_zero), 8);

        conn_client_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(conn_client_fd == -1){
            perror("conn_client_fd socket error");
            return 1;
        }
    }

    // 准备select相关内容
    fd_set ser_fdset;
    int max_fd=1;
    struct timeval ctl_time{2700,0};

//    link_list<fd_info> gui_client_list;
//    link_list<fd_info>* client_point;
    gui_client_list = new list<fd_info>;
    list<fd_info>::iterator client_iterator;
    bool select_flag = true,stdin_flag= true;
    if(connect(conn_client_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == 0)
    {
        printf("connect server successes\n");

        /** region #<折叠代码块># select前，将 ser_fdset 初始化 */
        FD_ZERO(&ser_fdset);

        //add server
        FD_SET(conn_client_fd, &ser_fdset);
        if(max_fd < conn_client_fd)
        {
            max_fd = conn_client_fd;
        }

        //add gui
        FD_SET(gui_server_fd, &ser_fdset);
        if(max_fd < gui_server_fd)
        {
            max_fd = gui_server_fd;
        }

        //add client 将client加入select列表中
        //这里一般只会有py的gui链接进来 实际上也只会有一个 但这么写也没问题 特别是和server形式比较相似
//        for(client_iterator = gui_client_list.begin();client_iterator!= gui_client_list.end();++client_iterator){
//            FD_SET(client_iterator->fd,&ser_fdset);
//            if(max_fd < client_iterator->fd)
//            {
//                max_fd = client_iterator->fd;
//            }
//        }
        /** endregion */

        while(select_flag)
        {


            //select多路复用
            int ret = select(max_fd + 1, &ser_fdset, nullptr, nullptr, &ctl_time);

            if(ret < 0){
                perror("select failure");
                continue;
            }

            else if(ret == 0){
                printf("time out!");
            }

            else{

                if(FD_ISSET(0,&ser_fdset))
                {
                    switch (EventStdinMsg()) {
                        case 0: {
                            break;
                        }
                        case -1: {
                            select_flag= false;
                            break;
                        }
                        case -2: {
                            FD_CLR(0,&ser_fdset);
                            stdin_flag= false;
                            break;
                        }
                    }
                }
                else if(stdin_flag){
                    FD_SET(0,&ser_fdset);
                }

                if(FD_ISSET(conn_client_fd, &ser_fdset))
                {
                    /** region ## 检查<conn_client_fd>是否存在于ser_fdset集合中,如果存在 意味着服务器发送了内容 ## */
                    if(EventServerMsg() == 0){
                        FD_CLR(conn_client_fd, &ser_fdset);
                        printf("server(%d) exit!\n", conn_client_fd);
                        // 广播到所有客户端
                        EventServerQuit();
                    }
                    /** endregion */
                }
                else{
                    FD_SET(conn_client_fd, &ser_fdset);
                }

                if(FD_ISSET(gui_server_fd, &ser_fdset)){
                    /** region ## 检查<gui_server_fd>是否存在于ser_fdset集合中，如果存在 意味着有GUI界面试图连接 ## */
                    EventNewGui();
                    /** endregion */
                } else{
                    FD_SET(gui_server_fd, &ser_fdset);
                }
            }

            /** region ## 逐个检查client列表中的套接字接口是否有信息 如果有则说明GUI发送了内容到这里等待转发 ## */
            for(client_iterator = gui_client_list->begin(); client_iterator != gui_client_list->end(); ++client_iterator){
                if(FD_ISSET(client_iterator->fd,&ser_fdset)){
                    if (EventGUIMsg(client_iterator)==0){
                        /** region ## 发现客户端连接退出后的一系列处理手段 ## */
                        FD_CLR(client_iterator->fd, &ser_fdset);
                        close(client_iterator->fd);
                        gui_client_list->erase(client_iterator++);
                        client_iterator--;
                        select_flag = false;
                        /** endregion */
                    }
                }
                else{
                    FD_SET(client_iterator->fd,&ser_fdset);
                    if(max_fd < client_iterator->fd){
                        max_fd = client_iterator->fd;
                    }
                }
            }
            /** endregion */
        }
    }
    gui_client_list->clear();
    delete gui_client_list;
    close(conn_client_fd);
    close(gui_server_fd);
    return 0;
}