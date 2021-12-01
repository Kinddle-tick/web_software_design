//
// Created by Kinddle Lee on 2021/11/27.
//
#include "client_source.h"
using namespace std;

int conn_client_fd,gui_server_fd;
using namespace std;
bool cue_flag = true;
//int logger_fd= 0;
//FILE *logger_fptr = nullptr;
fd_set * client_fd_set;
list<FdInfo>* gui_client_list;
list<FileSession>* file_list;
list<TimerSession*>* timer_list;
struct ClientSelfData * self_data;
int max_fd=1;
uint8_t client_timer_id=0;

const char* client_data_dir = "client_data/";

int main(int argc, const char * argv[]){
    struct sockaddr_in my_address{AF_INET};
    struct sockaddr_in server_address{};
    mkdir(client_data_dir,S_IRWXU);
    srand((unsigned int)time(nullptr)*100);
    if (argc!=3) {
        my_address.sin_family = AF_INET;
        my_address.sin_addr.s_addr=inet_addr("127.0.0.1");
        srand(time(nullptr)*100);
        my_address.sin_port = htons(random() % 30000 + 20000);
        bzero(&(my_address.sin_zero), 8);
//        cout<<"using default\n";
    }
    else{
        // C++程序本体维护的socket地址 从argv中获取
        my_address.sin_family = AF_INET;
        my_address.sin_port = htons(strtol(argv[2], nullptr, 10));
        my_address.sin_addr.s_addr=inet_addr(argv[1]);
        bzero(&(my_address.sin_zero), 8);
//        cout<<"using para from python\n";
    }

    //region gui_server 用来准备建立与gui的链接 需要提供一个自身的地址(my_address)
    //creat gui_server_fd socket 准备作为gui程序server的本体套接字
    if((gui_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        perror("creat gui_server_fd failure");
        return -1;
    }
    //gui_server_fd bind socket  绑定
    if(::bind(gui_server_fd, (const struct sockaddr *)&my_address, sizeof(my_address)) < 0){
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
    gui_client_list = new list<FdInfo>;
    file_list = new list<FileSession>;
    timer_list = new list<TimerSession*>;
    self_data = new ClientSelfData{.password = 12345,.state = kOffline};
    strcpy(self_data->confirmUserName,"default");
    strcat(self_data->client_data_dir_now,self_data->confirmUserName);
    bool select_flag = true,stdin_flag= true;
    struct timeval ctl_time{0,200000};//0.2s一轮
//    logger_fd = open("client_logger",O_TRUNC|O_WRONLY|O_CREAT,S_IXUSR|S_IWUSR|S_IRUSR);
//    logger_fptr = fdopen(logger_fd,"w");

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
            cout << "(" << kStateDescription[self_data->state] << ") shell \033[35m" << self_data->confirmUserName << "\033[0m % ";
            cout.flush();
        }
        cout.flush();
        int ret = select(max_fd + 1, client_fd_set, nullptr, nullptr, &ctl_time);

        if(ret > 0){

            if(conn_client_fd > 0){
                if(FD_ISSET(conn_client_fd, client_fd_set))
                {
                    /** region ## 检查<conn_client_fd>是否存在于ser_fdset集合中,如果存在 意味着服务器发送了内容 ## */
//                    EventServerMessage();
                    switch (EventServerMessage()) {
                        case kServerShutdown:
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
                    if (EventGuiMessage(&*client_iterator) == 0){
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

            //stdin中可能会更改client_fd_set状态 所以通常放最后处理
            if(FD_ISSET(0,client_fd_set))
            {
                switch (EventStdinMessage()) {
                    case kNormal:
                        cue_flag = true;
                        break;

                    case kClientExit:
                        select_flag= false; // stdin输入指令使其退出
                        break;

                    case kStdinExit:
                        FD_CLR(0,client_fd_set);
                        stdin_flag= false;
                        break;

                    case kConnectServer:
                        break;
                }
            }
            else if(stdin_flag){
                FD_SET(0,client_fd_set);
            }

        }

        else if(ret == 0){
//            cout<<"[debug] timeout"<<endl;
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

            for(auto timer_iter = timer_list->begin();timer_iter!=timer_list->end();){
                if((*timer_iter)->get_timer_state_()==kTimerDisable){
#if CLIENT_DEBUG_LEVEL>2
                    printf("清理了一个定时器(%d)\n",(*timer_iter)->timer_id_);
#endif
                    timer_iter = timer_list->erase(timer_iter);
                }
                else{
                    timer_iter++;
                }
            }

            for(auto &timer_iter:*timer_list){
                if(timer_iter->TimeoutJustice()){
                    timer_iter->TimerTrigger();
                    timer_iter->TimerUpdate();
                    fflush(stdout);
                }
            }

        }

        else{
            perror("select failure");
            continue;
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
