#include "My_console.h"
#include <list>

struct client_info{
    int fd;
    int state=0;
    clock_t tick=0;
    char nickname[21];
};

const char * server_addr_Decimal = "127.0.0.1";
const int server_port = 4000;

#define backlog 16


using namespace std;
int client_ID = 0;
int conn_server_fd = 0;
list<client_info>* client_list;

//declare
unsigned int ActionCHAPChallenge(int);

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

        uint answer = ActionCHAPChallenge(client_sock_fd);
        printf("CHAP answer is:%u\n",answer);
        fflush(stdout);

        delete tmp_client;
    }
    return client_sock_fd;
}

int EventClientMsg(list<client_info>::iterator client){
    char receive_message[HEADER_SIZE + BUFFER_SIZE]={0};
    bzero(receive_message, HEADER_SIZE + BUFFER_SIZE);
    ssize_t byte_num=read(client->fd, receive_message, HEADER_SIZE + BUFFER_SIZE);
    if(byte_num >0){
        printf("message form client(%s):%s\n", client->nickname, receive_message + HEADER_SIZE);

        // 广播到所有客户端
        for(auto client_point = client_list->begin(); client_point != client_list->end(); ++client_point){
            if(client_point->fd != client->fd){
                printf("send message to client(%s).fd=%d with %s\n",
                       client_point->nickname, client_point->fd, receive_message + HEADER_SIZE);
                send(client_point->fd, receive_message, strlen(receive_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
            }else if(client_list->size() == 1){
                printf("echo message to client(%s).fd=%d with %s\n",
                       client_point->nickname, client_point->fd, receive_message + HEADER_SIZE);
                send(client_point->fd, receive_message, strlen(receive_message + HEADER_SIZE) + HEADER_SIZE + 1, 0);
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

unsigned int ActionCHAPChallenge(int client_fd){
    char packet_total[HEADER_SIZE + BUFFER_SIZE]={};
    clock_t seed = clock();
    srand(seed);//这里用time是最经典的 但是因为服务端总是要等很久 每次连接的时间其实差别很大 所以用clock()我觉得不会有问题
    uint8_t  one_data_size = rand() % 3; // 2**one_data_size arc4random()是一个非常不错的替代解法 但是这边还是用了rand先
    uint32_t data_length = 10 + rand()%(BUFFER_SIZE/(1<<one_data_size )- 10);//至少要有10个数据 （10～40字节）
    header packet_header{};
    packet_header.chap_proto.proto = PROTO_CHAP;
    packet_header.chap_proto.one_data_size=one_data_size;
    packet_header.chap_proto.data_length=data_length;
    memcpy(packet_total, packet_header.chr, sizeof(header));
    union num{
        int32_t int32;
        uint32_t uint32;
        uint16_t uint16;
        uint8_t uint8;
        uint8_t bit[4];
    };
    unsigned int answer=0;
    for(int i = 0;i < data_length ; i++){
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
        cout<<answer<<endl;
    }

    send(client_fd, packet_total, HEADER_SIZE + data_length * (1<<one_data_size), 0);
    if(one_data_size==2){
        return answer;
    } else{
        return answer % (1<<(8*(1<<one_data_size)));
    }
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
    fd_set ser_fd_set{0};
    int max_fd=1;
    bool select_flag = true,stdin_flag= true;

    struct timeval ctl_time{2700, 0};

    client_list = new list<client_info>;
    list<client_info>::iterator client_iterator;

    //add standard input
    FD_SET(0,&ser_fd_set);

    //add server
    FD_SET(conn_server_fd, &ser_fd_set);
    if(max_fd < conn_server_fd)
    {
        max_fd = conn_server_fd;
    }

    printf("wait for client connnect!\n");
    while(select_flag)
    {
        int ret = select(max_fd + 1, &ser_fd_set, nullptr, nullptr, &ctl_time);

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
            if(FD_ISSET(0,&ser_fd_set)) //<标准输入>是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                switch (EventRcvStdin()) {
                    case 0: break;

                    case 1:{
                        select_flag = false;
                        break;
                    }
                    case 2:{
                        FD_CLR(0,&ser_fd_set);
                        stdin_flag= false;
                        break;
                    }
                }
            }
            else if(stdin_flag){
                FD_SET(0,&ser_fd_set);
            }

            if(FD_ISSET(conn_server_fd, &ser_fd_set))//<server>是否存在于ser_fdset集合中,如果存在 意味着有connect请求
            {
                EventNewClient();
            }
            else{
                FD_SET(conn_server_fd, &ser_fd_set);
            }

        }

        //deal with the message
        for(client_iterator = client_list->begin(); client_iterator != client_list->end(); ++client_iterator){
            if(FD_ISSET(client_iterator->fd,&ser_fd_set)){
                if(EventClientMsg(client_iterator)==-1){
                    FD_CLR(client_iterator->fd, &ser_fd_set);
                    close(client_iterator->fd);
                    client_list->erase(client_iterator++);
                    client_iterator--;
                }
            }
            else{
                FD_SET(client_iterator->fd,&ser_fd_set);
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

