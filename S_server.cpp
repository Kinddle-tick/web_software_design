#include "My_console.h"

template<typename T>
class link_list{
public:
    link_list * previous= nullptr;
    link_list * next = nullptr;
    T * value;
    link_list():value(nullptr){};
    explicit link_list(T* member):value(member){}

    bool is_head(){
        return this->value == nullptr;
    }

    void insert(T* new_member){
        auto* tmp = new link_list(new_member);
        tmp->next = this->next;
        tmp->previous = this;
        if(this->next != nullptr)
            this->next->previous=tmp;
        this->next = tmp;
    }

    void append(T* new_member){ //从末尾添加 实际上实现是有些许冗余的..
        link_list* tmp = this->move(this->len() - this->index() -1);
        tmp->insert(new_member);
    }

    void pop(bool delete_value = true){
        if(this->next!= nullptr)
            this->next->previous = this->previous;
        this->previous->next = this->next;
        if(delete_value){
            delete value;
        }
        delete this;
    }

    void redirect(T* new_member){
        this->value = new_member;
    }

    int index(){
        int num = -1;
        link_list* tmp = this;
        while (!tmp->is_head()){
            tmp = tmp->previous;
            num++;
        }
        return num;
    }

    int len(){
        int num = index();
        link_list* tmp = this;
        while (tmp->next != nullptr){
            tmp = tmp->next;
            num++;
        }
        return num+1;
    }

    link_list* move(int step){
        if(step+index()<-1 || step+index()>len()){
            return nullptr; //超界
        }
        link_list* tmp = this;
        while(step){
            if(step>0){
                tmp = tmp->next;
                step--;
            }
            else{
                tmp = tmp->previous;
                step++;
            }
        }
        return tmp;
    }

};

struct client_info{
    int fd;
    char nickname[21];
};

const char * server_addr_Decimal = "127.0.0.1";
const int server_port = 4000;

#define BUFF_SIZE 1024
#define backlog 16
#define SER_PORT 11272

int main(int agrc,char **argv)
{
    int ser_souck_fd;
    char input_message[BUFF_SIZE];
    char resv_message[BUFF_SIZE];


    struct sockaddr_in ser_addr{};
    ser_addr.sin_family= AF_INET;    //IPV4
    ser_addr.sin_port = htons(SER_PORT);
    ser_addr.sin_addr.s_addr = INADDR_ANY;  //指定的是所有地址

    //creat server socket 准备server的本体套接字
    if( (ser_souck_fd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
    {
        perror("creat failure");
        return -1;
    }

    //bind socket  绑定
    if(bind(ser_souck_fd, (const struct sockaddr *)&ser_addr,sizeof(ser_addr)) < 0)
    {
        perror("bind failure");
        return -1;
    }

    //listen 监听
    if(listen(ser_souck_fd, backlog) < 0)
    {
        perror("listen failure");
        return -1;
    }


    //fd_set  准备fd_set
    fd_set ser_fdset;
    int max_fd=1;
    struct timeval mytime{};
    printf("wait for client connnect!\n");

    //client_list
    link_list<client_info> client_list;
    link_list<client_info>* client_point;

    int client_ID = 0;
    while(true)
    {
        mytime.tv_sec=2700;
        mytime.tv_usec=0;

        FD_ZERO(&ser_fdset); //

        //add standard input
        FD_SET(0,&ser_fdset);
        if(max_fd < 0)
        {
            max_fd=0;
        }

        //add server
        FD_SET(ser_souck_fd,&ser_fdset);
        if(max_fd < ser_souck_fd)
        {
            max_fd = ser_souck_fd;
        }

        //add client 将client加入select列表中
        for(client_point = client_list.next;client_point!= nullptr;client_point=client_point->next){
            FD_SET(client_point->value->fd,&ser_fdset);
            if(max_fd < client_point->value->fd)
                {
                    max_fd = client_point->value->fd;
                }
        }

        //select多路复用
        int ret = select(max_fd + 1, &ser_fdset, nullptr, nullptr, &mytime);

        if(ret < 0)
        {
            perror("select failure\n");
            continue;
        }

        else if(ret == 0)
        {
            printf("time out!");
            break;
        }

        else
        {
            if(FD_ISSET(0,&ser_fdset)) //<标准输入>是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                bzero(input_message,BUFF_SIZE); // 将前n个字符串清零
                fgets(input_message,BUFF_SIZE,stdin);
                if(strcmp(input_message,"exit\n")==0){
                    break;
                }

                for(client_point = client_list.next;client_point!= nullptr;client_point=client_point->next){
                    printf("send message to client(%s).fd = %d\n", client_point->value->nickname, client_point->value->fd);
                    send(client_point->value->fd, input_message, strlen(input_message), 0);
                }

            }

            if(FD_ISSET(ser_souck_fd, &ser_fdset))//<server>是否存在于ser_fdset集合中,如果存在 意味着有connect请求
            {
                struct sockaddr_in client_address{};
                socklen_t address_len;
                int client_sock_fd = accept(ser_souck_fd,(struct sockaddr *)&client_address, &address_len);
                if(client_sock_fd > 0)
                {
                    char nickname_tmp[21]{0};
                    snprintf(nickname_tmp,20,"User[%d]",client_ID++);
                    auto * tmp_client = new client_info{client_sock_fd};
                    strcpy(tmp_client->nickname,nickname_tmp);
                    client_list.append(tmp_client);
                    printf("client(%s) joined!\n",tmp_client->nickname);

                }
            }

        }

        //deal with the message
        for(client_point = client_list.next;client_point!= nullptr;client_point=client_point->next){
            if(FD_ISSET(client_point->value->fd,&ser_fdset)){
                bzero(resv_message,BUFF_SIZE);
                ssize_t byte_num=read(client_point->value->fd,resv_message,BUFF_SIZE);
                if(byte_num >0){
                    printf("message form client(%s):%s\n", client_point->value->nickname, resv_message);
                    // 广播到所有客户端
                    link_list<client_info>* client_point_tmp = client_point;
                    for(client_point = client_list.next;client_point!= nullptr;client_point=client_point->next){
                        if(client_point != client_point_tmp){
                            printf("send message to client(%s).fd=%d with %s\n",
                                   client_point->value->nickname, client_point->value->fd, resv_message);
                            send(client_point->value->fd, resv_message, strlen(resv_message), 0);
                        }else if(client_list.len()==1){
                            printf("echo message to client(%s).fd=%d with %s\n",
                                   client_point->value->nickname, client_point->value->fd, resv_message);
                            send(client_point->value->fd, resv_message, strlen(resv_message), 0);

                        }

                    }
                    client_point = client_point_tmp;

                }
                else if(byte_num < 0){
                    printf("recv error!");
                } else{
                    FD_CLR(client_point->value->fd, &ser_fdset);
                    printf("client(%s) exit!\n",client_point->value->nickname);
                    close(client_point->value->fd);
                    client_point = client_point->previous;
                    client_point->next->pop();
                    continue;  //这里如果用break的话一个客户端退出会造成服务器也退出。
                }
            }
        }
    }

    printf("exiting...");
    close(ser_souck_fd);
    return 0;
}

