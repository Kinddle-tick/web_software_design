//
// Created by Kinddle Lee on 2021/11/27.
//

#include "server_source.h"

using namespace std;
int conn_server_fd = 0;
unsigned int session_id = 1;
list<user_info>* user_list;
list<client_session>* client_list;
list<chap_session>* chap_list;
list<file_session>* file_list;

fd_set ser_fd_set{0};
int max_fd=1;
const char server_data_dir[DIR_LENGTH] = "server_data/";

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
