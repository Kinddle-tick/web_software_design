#include "My_console.h"
#include <list>

//template<typename T>
//class link_list{
//public:
//    link_list * previous= nullptr;
//    link_list * next = nullptr;
//    T * value;
//    link_list():value(nullptr){};
//    explicit link_list(T* member):value(member){}
//
//    bool is_head(){
//        return this->value == nullptr;
//    }
//
//    void insert(T* new_member){
//        auto* tmp = new link_list(new_member);
//        tmp->next = this->next;
//        tmp->previous = this;
//        if(this->next != nullptr)
//            this->next->previous=tmp;
//        this->next = tmp;
//    }
//
//    void append(T* new_member){ //从末尾添加 实际上实现是有些许冗余的..
//        link_list* tmp = this->move(this->len() - this->index() -1);
//        tmp->insert(new_member);
//    }
//
//    void pop(bool delete_value = true){
//        if(this->next!= nullptr)
//            this->next->previous = this->previous;
//        this->previous->next = this->next;
//        if(delete_value){
//            delete value;
//        }
//        delete this;
//    }
//
//    void redirect(T* new_member){
//        this->value = new_member;
//    }
//
//    int index(){
//        int num = -1;
//        link_list* tmp = this;
//        while (!tmp->is_head()){
//            tmp = tmp->previous;
//            num++;
//        }
//        return num;
//    }
//
//    int len(){
//        int num = index();
//        link_list* tmp = this;
//        while (tmp->next != nullptr){
//            tmp = tmp->next;
//            num++;
//        }
//        return num+1;
//    }
//
//    link_list* move(int step){
//        if(step+index()<-1 || step+index()>len()){
//            return nullptr; //超界
//        }
//        link_list* tmp = this;
//        while(step){
//            if(step>0){
//                tmp = tmp->next;
//                step--;
//            }
//            else{
//                tmp = tmp->previous;
//                step++;
//            }
//        }
//        return tmp;
//    }
//
//};

struct client_info{
    int fd;
    int state=0;
    clock_t tick=0;
    char nickname[21];
};

const char * server_addr_Decimal = "127.0.0.1";
const int server_port = 4000;

#define HEADER_SIZE 20
#define BUFFER_SIZE 1004
#define backlog 16
#define SER_PORT 11271

using namespace std;
int client_ID = 0;
int conn_server_fd = 0;
list<client_info>* client_list;

int EventRcvStdin(){
    char input_message[HEADER_SIZE+BUFFER_SIZE]={0};
    bzero(input_message, HEADER_SIZE+BUFFER_SIZE); // 将前n个字符串清零
    fgets(input_message+HEADER_SIZE, BUFFER_SIZE, stdin);
    if(strcmp(input_message+HEADER_SIZE,"exit\n")==0){
        return 1;
    }
    if(strlen(input_message+HEADER_SIZE)==0) {
        printf("发现处于后台工作 stdin被抑制...\n");
        return 2;
    }
    for(auto & client_iterator : *client_list){
        printf("send message to client(%s).fd = %d\n", client_iterator.nickname,client_iterator.fd);
        send(client_iterator.fd, input_message, strlen(input_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
    }
    return 0;
}

int EventNewClient(){
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(conn_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        char nickname_tmp[21]{0};
        snprintf(nickname_tmp,20,"User[%d]",client_ID++);
        auto* tmp_client = new client_info{client_sock_fd,0,clock()};
        strcpy(tmp_client->nickname,nickname_tmp);
        client_list->push_back(*tmp_client);
        printf("client(%s) joined!\n",tmp_client->nickname);
        delete tmp_client;
    }
    return client_sock_fd;
}

int EventClientMsg(list<client_info>::iterator client){
    char recv_message[HEADER_SIZE + BUFFER_SIZE]={0};
    bzero(recv_message, HEADER_SIZE + BUFFER_SIZE);
    ssize_t byte_num=read(client->fd, recv_message, HEADER_SIZE+BUFFER_SIZE);
    if(byte_num >0){
        printf("message form client(%s):%s\n", client->nickname, recv_message + HEADER_SIZE);
        // 广播到所有客户端
        for(auto client_point = client_list->begin(); client_point != client_list->end(); ++client_point){
            if(client_point->fd != client->fd){
                printf("send message to client(%s).fd=%d with %s\n",
                       client_point->nickname, client_point->fd, recv_message + HEADER_SIZE);
                send(client_point->fd, recv_message, strlen(recv_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
            }else if(client_list->size() == 1){
                printf("echo message to client(%s).fd=%d with %s\n",
                       client_point->nickname, client_point->fd, recv_message + HEADER_SIZE);
                send(client_point->fd, recv_message, strlen(recv_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
            }
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

int ActionCHAPChallenge(list<client_info>::iterator client){

}

clock_t time_ms(clock_t a,clock_t b){
    if(a-b >0){
        return (a-b)/CLOCKS_PER_SEC;
    }else{
        return (b-a)/CLOCKS_PER_SEC;
    }
}

int main(int agrc,char **argv)
{

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


    //fd_set  准备fd_set
    fd_set ser_fdset{0};
    int max_fd=1;
    struct timeval ctl_time{2700, 0};
    bool select_flag = true,stdin_flag= true;
    client_list = new list<client_info>;
    list<client_info>::iterator client_iterator;

    FD_ZERO(&ser_fdset);

    //add standard input
    FD_SET(0,&ser_fdset);

    //add server
    FD_SET(conn_server_fd, &ser_fdset);
    if(max_fd < conn_server_fd)
    {
        max_fd = conn_server_fd;
    }

    printf("wait for client connnect!\n");
    while(select_flag)
    {
        int ret = select(max_fd + 1, &ser_fdset, nullptr, nullptr, &ctl_time);

        if(ret < 0)
        {
            perror("select failure\n");
            continue;
        }
        else if(ret == 0)
        {
            printf("time out!");
        }
        else
        {
            if(FD_ISSET(0,&ser_fdset)) //<标准输入>是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                switch (EventRcvStdin()) {
                    case 0: break;

                    case 1:{
                        select_flag = false;
                        break;
                    }
                    case 2:{
                        FD_CLR(0,&ser_fdset);
                        stdin_flag= false;
                        break;
                    }
                }
            }
            else if(stdin_flag){
                FD_SET(0,&ser_fdset);
            }

            if(FD_ISSET(conn_server_fd, &ser_fdset))//<server>是否存在于ser_fdset集合中,如果存在 意味着有connect请求
            {
                EventNewClient();
            }
            else{
                FD_SET(conn_server_fd, &ser_fdset);
            }

        }

        //deal with the message
        for(client_iterator = client_list->begin(); client_iterator != client_list->end(); ++client_iterator){
            if(FD_ISSET(client_iterator->fd,&ser_fdset)){
                if(EventClientMsg(client_iterator)==-1){
                    FD_CLR(client_iterator->fd, &ser_fdset);
                    close(client_iterator->fd);
                    client_list->erase(client_iterator++);
                    client_iterator--;
                }
            }
            else{
                FD_SET(client_iterator->fd,&ser_fdset);
                if(max_fd < client_iterator->fd){
                        max_fd = client_iterator->fd;
                    }
            }
        }
    }

    printf("exiting...");
    close(conn_server_fd);

    client_list->clear();
    delete client_list;
    return 0;
}

