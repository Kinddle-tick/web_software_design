//
// Created by Kinddle Lee on 2021/11/27.
//

#include "server_source.h"
using namespace std;

bool ErrorSimulator(int prob=0){
    if(random()%100 < prob){
        return true;
    }
    else{
        return false;
    }
}

int EventReceiveStdin(){
    char input_message[kBufferSize - sizeof(UserNameString)]={0};
    bzero(input_message, kBufferSize - sizeof(UserNameString)); // 将前n个字符串清零
    fgets(input_message, kBufferSize - sizeof(UserNameString), stdin);
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
        char packet_total[kHeaderSize + kBufferSize]={0};
        auto* packet_header = (header*)packet_total;
        auto* packet_data = (data*)(packet_total + kHeaderSize);
        packet_header->proto = kProtoMessage;
        strcpy(packet_data->msg_general.userName,"");
        strcpy(packet_data->msg_general.msg_data,input_message);
        packet_header->msg_proto.data_length= htonl(sizeof(UserNameString) + strlen(input_message) + 1);
        send(client_iterator.socket_fd, input_message, kHeaderSize + ntohl(packet_header->msg_proto.data_length), 0);
    }
    return 0;
}

int EventNewClient(){
    struct sockaddr_in client_address{};
    socklen_t address_len;
    int client_sock_fd = accept(conn_server_fd, (struct sockaddr *)&client_address, &address_len);
    if(client_sock_fd > 0)
    {
        ClientSession tmp_cs = {.socket_fd = client_sock_fd,
                .state = kOffline};
        clock_gettime(CLOCK_MONOTONIC,&tmp_cs.tick);
        strcpy(tmp_cs.nickname,"Unknown");
        client_list->push_back(tmp_cs);
        printf("client-kOffline joined in socket_fd(%d)!\n",client_sock_fd);
        fflush(stdout);
    }
    return client_sock_fd;
}

int EventClientMessage(ClientSession* client){
//    printf("debug:收到客户端fd(%d)信息",client->socket_fd);
    char receive_message[kHeaderSize + kBufferSize]={0};
    auto* packet_header = (header *)(receive_message);
    ssize_t byte_num = read(client->socket_fd, receive_message, kHeaderSize);
    byte_num += read(client->socket_fd, receive_message + kHeaderSize, ntohl(packet_header->base_proto.data_length));
    if(byte_num >0){
        auto* receive_packet_header = (header*) receive_message;
        switch (receive_packet_header->proto) {
            case kProtoControl:
                switch (receive_packet_header->ctl_proto.ctl_code) {
                    case kControlLogin:
                        ActionControlLogin(receive_message,client);
                        break;
                    case kControlRegister:
                        break;
                    case kControlChangeUsername:
                        break;
                    case kControlChangePassword:
                        break;
                    case kControlLs:
                        ActionControlLsResponse(receive_message,client);
                        break;
                    default:
                        cout<<"receive invalid CTL_code";
                        break;
                }
                return kProtoControl;
            case kProtoMessage:
                ActionMessageProcessing(receive_message, client);
                return kProtoMessage;
            case kProtoChap:
                switch (receive_packet_header->chap_proto.chap_code) {
                    case kChapResponse:
                        ActionChapJustice(receive_message, client);
                        break;
                    default:
                        printf("\tInvalid CHAP_CODE\n");
                }
                return kProtoChap;
            case kProtoFile:
                switch (receive_packet_header->file_proto.file_code) {
                    case kFileRequest:
                        ActionFileResponse(receive_message,client);
                        break;
                    case kFileAck:
                        ActionFileAckReceived(receive_message,client);
                        break;
                    default:
                        break;
                }
                return kProtoFile;
            case kProtoGeneralAck:
                ActionGeneralAckReceive(receive_message,client);
                return kProtoGeneralAck;
            case kProtoGeneralFinish:
                ActionGeneralFinishReceive(receive_message,client);
                return kProtoGeneralFinish;
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

int ActionGeneralFinishSend(const char * receive_packet_total,ClientSession * client){
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralFinish,
            .detail_code =0,
            .timer_id_tied = client->server_timer_id = client->server_timer_id % 255 + 1,
            .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
            .data_length=htonl(0),
    }};
    auto* tmp_timer = new TimerRetranslationSession(5,send_packet_header.base_proto.timer_id_tied,client->socket_fd,
                                                    (const char *)&send_packet_header,kHeaderSize+ntohl(send_packet_header.chap_proto.data_length));
    tmp_timer->set_retry_max(3);
    timer_list->push_back(tmp_timer);
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(client->socket_fd,&send_packet_header,kHeaderSize+ntohl(send_packet_header.base_proto.data_length),0);
    }
    return 0;
}
int ActionGeneralFinishReceive(const char *receive_packet_total,ClientSession * client){
    ActionGeneralAckSend(receive_packet_total,client);
    return 0;
}

int ActionGeneralAckSend(const char * receive_packet_total, ClientSession * client){
    auto* receive_packet_header = (header*)receive_packet_total;
    header send_packet_header{.base_proto{.proto=kProtoGeneralAck,
                                          .detail_code = receive_packet_header->base_proto.detail_code,
                                          .timer_id_tied = 0,
                                          .timer_id_confirm=receive_packet_header->base_proto.timer_id_tied,
                                          .data_length = htonl(0)
            }};
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    if(!ErrorSimulator(kGenericAckProb)){
        send(client->socket_fd, &send_packet_header, kHeaderSize + ntohl(send_packet_header.chap_proto.data_length), 0);
    }
    return 0;
}
int ActionGeneralAckReceive(const char * receive_packet_total, ClientSession * client){
    auto* receive_packet_header = (header*)receive_packet_total;
    TimerDisable(receive_packet_header->base_proto.timer_id_confirm,client->socket_fd);
    return 0;
}

int ActionMessageProcessing(const char* receive_packet_total, ClientSession* client){
    auto* packet_header = (header*)receive_packet_total;
    auto* packet_data = (data*)(receive_packet_total + kHeaderSize);
//    UserNameString from_user = {0};
//    strcpy(from_user,packet_data->msg_general.userName);

    printf("message form client(%s):%s\n", client->nickname, packet_data->msg_general.msg_data );
    packet_header->msg_proto.data_length = htonl(sizeof(UserNameString) + strlen(packet_data->msg_general.msg_data));
    bool flood_flag = (strcmp(packet_data->msg_general.userName,"") == 0);
    strcpy(packet_data->msg_general.userName,client->nickname);

    // 广播到所有客户端
    for(auto client_point = client_list->begin(); client_point != client_list->end(); ++client_point){
        if(flood_flag){
            if(client_point->socket_fd != client->socket_fd){
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     kHeaderSize + ntohl(packet_header->msg_proto.data_length), 0);
            }else if(client_list->size() == 1){
                printf("echo message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     kHeaderSize + ntohl(packet_header->msg_proto.data_length), 0);
            }
        }
        else{
            if(strcmp(client_point->nickname,packet_data->msg_general.userName) == 0){
                printf("send message to client(%s).socket_fd=%d with %s\n",
                       client_point->nickname, client_point->socket_fd, packet_data->msg_general.msg_data);
                send(client_point->socket_fd, receive_packet_total,
                     kHeaderSize + ntohl(packet_header->msg_proto.data_length), 0);
            }
        }

    }
    return 0;
}

int ActionControlLogin(const char* packet_total, ClientSession* client){
    auto * packet_data  = (data *)(packet_total + kHeaderSize);
    for(auto & user_iter : *user_list){
        if(!strcmp(user_iter.user_name,packet_data->ctl_login.userName)){
            strcpy(client->nickname, packet_data->ctl_login.userName);
            strcpy(client->now_path, user_iter.root_path);
            ActionChapChallenge(packet_total,client);
            client->state = kChapChallenging;
            return 0;
        }
    }
    cout<<"没找到用户名：("<<packet_data->ctl_login.userName<<")"<<endl;
    ActionControlUnregistered(packet_total,client);
    client->state = kOffline;
    return 1;
}

unsigned int ActionControlUnregistered(const char *receive_packet_total,ClientSession* client){
    char send_packet[kHeaderSize + kBufferSize];
    auto * send_packet_header = (header*) send_packet;
    send_packet_header->proto = kProtoControl;
    send_packet_header->ctl_proto.ctl_code=kControlUnregistered;
    send_packet_header->ctl_proto.data_length = htonl(0);
    send(client->socket_fd, send_packet, kHeaderSize + ntohl(send_packet_header->ctl_proto.data_length), 0);
    return 0;
}

int ActionControlLsResponse(const char *receive_packet_total,ClientSession* client){
    if(client->state == kOnline){
        DIR *dir = opendir(client->now_path);
        if(dir == nullptr){
            cout<<"client ("<<client->nickname<<")的ls请求被拒绝: 用户根目录无效"<<endl;
            return -1;
        }
        char response_packet[kHeaderSize + kBufferSize]= {0};
        auto * response_header = (header*)response_packet;
        auto * response_data = (data*)(response_packet + kHeaderSize);
        response_header->proto = kProtoControl;
        response_header->ctl_proto.ctl_code = kControlLs;
        dirent* dir_ptr;
        while((dir_ptr = readdir(dir))!= nullptr){
            strcat(response_data->ctl_ls.chr,dir_ptr->d_name);
            strcat(response_data->ctl_ls.chr,"\n");
        }
        closedir(dir);
        response_header->ctl_proto.data_length =  htonl(strlen(response_data->ctl_ls.chr));
        send(client->socket_fd, response_packet, kHeaderSize + ntohl(response_header->ctl_proto.data_length), 0);
        return 0;
    } else{
        cout<<"client ("<<client->nickname<<")的ls请求被拒绝: 非Online状态没有权限"<<endl;
        return -1;
    }
}

unsigned int ActionChapChallenge(const char *receive_packet_total,ClientSession* client){
    auto* receive_packet_header = (header*)receive_packet_total;
    char send_packet_total[kHeaderSize + kBufferSize]={};
    clock_t seed = clock();
    srand(seed);//这里用time是最经典的 但是因为服务端总是要等很久 每次连接的时间其实差别很大 所以用clock()我觉得不会有问题
    uint8_t  one_data_size = random() % 3; // 2**one_data_size arc4random()是一个非常不错的替代解法 但是这边还是用了rand先
    uint32_t number_count = 10 + random() % (kBufferSize / (1 << one_data_size ) - 10);//至少要有10个数据 （10～40个字节）

    auto sequence = (uint32_t)(random() % ((0U - 1)/4 - chap_list->size()));
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

    auto* send_packet_header=(header *)send_packet_total;
    send_packet_header->chap_proto.proto = kProtoChap;
    send_packet_header->chap_proto.chap_code = kChapChallenging;
    send_packet_header->chap_proto.one_data_size=one_data_size;
    send_packet_header->chap_proto.number_count=htonl(number_count);
    send_packet_header->chap_proto.sequence = htonl(sequence);
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
        num tmpnum{.int32=(int32_t)random()};
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

        memcpy(&(send_packet_total[kHeaderSize + i * (1 << one_data_size)]), &tmpnum, 1 << one_data_size);
        answer += one_data_size==0 ? tmpnum.uint8:0;
        answer += one_data_size==1 ? ntohs(tmpnum.uint16):0;
        answer += one_data_size==2 ? ntohl(tmpnum.uint32):0;
    }
    send_packet_header->chap_proto.data_length = htonl(number_count * (1 << one_data_size));

    send_packet_header->chap_proto.timer_id_tied = client->server_timer_id = client->server_timer_id % 255 + 1;
    send_packet_header->chap_proto.timer_id_confirm = receive_packet_header->chap_proto.timer_id_tied;
    auto* tmp_timer = new TimerRetranslationSession(5,send_packet_header->chap_proto.timer_id_tied,client->socket_fd,
                                                    send_packet_total,kHeaderSize + ntohl(send_packet_header->chap_proto.data_length));
    timer_list->push_back(tmp_timer);
    TimerDisable(receive_packet_header->chap_proto.timer_id_confirm,client->socket_fd);

    ChapSession tmp_chap_session{.sequence=sequence,.answer=answer,.client = client};
    clock_gettime(CLOCK_MONOTONIC,&tmp_chap_session.tick);
    chap_list->insert(chap_iterator, tmp_chap_session);
    if(!ErrorSimulator(kGenericErrorProb)){
        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->chap_proto.data_length), 0);
    }

    return answer;
}

unsigned int ActionChapJustice(const char* receive_packet_total, ClientSession* client){
    auto* receive_packet_header = (header *)(receive_packet_total);
    auto* receive_packet_data = (data *)(receive_packet_total + kHeaderSize);
    char send_packet_total[kHeaderSize];
    auto* send_packet_header = (header*)send_packet_total;
    memcpy(send_packet_total, receive_packet_total, kHeaderSize);

    uint32_t sequence = ntohl(receive_packet_header->chap_proto.sequence);
    send_packet_header->chap_proto.sequence=htonl(sequence+1);

    for (auto chap_iter=chap_list->begin();chap_iter!= chap_list->end();++chap_iter) {
        if(chap_iter->sequence == ntohl(receive_packet_header->chap_proto.sequence) - 1){
            for(auto & user_iter : *user_list){
                if(!strcmp(user_iter.user_name,receive_packet_data->chap_response.userName)&&
                   !strcmp(user_iter.user_name,chap_iter->client->nickname)){
                    if (ntohl(receive_packet_data->chap_response.answer)==(chap_iter->answer^user_iter.password)){
                        send_packet_header->chap_proto.chap_code=kChapSuccess;
                        strcpy(client->nickname,receive_packet_data->chap_response.userName);
                        send_packet_header->chap_proto.data_length = htonl(0);

                        send_packet_header->chap_proto.timer_id_tied = client->server_timer_id = client->server_timer_id%255+1;
                        send_packet_header->chap_proto.timer_id_confirm = receive_packet_header->chap_proto.timer_id_tied;
                        auto* tmp_timer = new TimerRetranslationSession(5,send_packet_header->chap_proto.timer_id_tied,client->socket_fd,
                                                                        send_packet_total,kHeaderSize + ntohl(send_packet_header->chap_proto.data_length));
                        timer_list->push_back((tmp_timer));
                        TimerDisable(receive_packet_header->chap_proto.timer_id_confirm,client->socket_fd);
                        if(!ErrorSimulator(kGenericErrorProb)){
                            send(client->socket_fd, send_packet_total,
                                 kHeaderSize + ntohl(send_packet_header->chap_proto.data_length), 0);
                        }
                        printf("\tCHAP_SUCCESS,\"%s\" login in socket_fd(%d)\n",client->nickname,client->socket_fd);
                        client->state=kOnline;
                    } else{
                        send_packet_header->chap_proto.chap_code=kChapFailure;
                        send_packet_header->chap_proto.data_length = htonl(0);

                        send_packet_header->chap_proto.timer_id_tied = client->server_timer_id = client->server_timer_id%255+1;
                        send_packet_header->chap_proto.timer_id_confirm = receive_packet_header->chap_proto.timer_id_tied;
                        auto* tmp_timer = new TimerRetranslationSession(5,send_packet_header->chap_proto.timer_id_tied,client->socket_fd,
                                                                        send_packet_total, kHeaderSize + ntohl(send_packet_header->chap_proto.data_length));
                        timer_list->push_back((tmp_timer));
                        TimerDisable(receive_packet_header->chap_proto.timer_id_confirm,client->socket_fd);
                        if(!ErrorSimulator(kGenericErrorProb)){
                            send(client->socket_fd, send_packet_total,
                                 kHeaderSize + ntohl(send_packet_header->chap_proto.data_length), 0);
                        }
                        printf("\tCHAP_FAILURE in socket_fd(%d)\n",client->socket_fd);
                        client->state=kOffline;
                    }
                    printf("\t清除CHAP_challenge记录：sequence:%u answer:%u\n",chap_iter->sequence,chap_iter->answer);
                    chap_list->erase(chap_iter);
                    return 0;
                }
            }
            printf("\tCHAP_ERROR:未搜寻到CHAP请求的用户名\n");
            client->state=kOffline;
            return 2;
        }
    }
    printf("\tCHAP_ERROR:未找到对应的CHAP_challenge记录\n");
    client->state=kOffline;
    return 1;
}

int ActionFileResponse(const char* receive_packet_total, ClientSession* client){
    auto* receive_packet_header = (header *)(receive_packet_total);
    auto* receive_packet_data = (data *)(receive_packet_total + kHeaderSize);
    cout<<"请求文件名:"<<receive_packet_data->file_request.file_path<<endl;
    cout<<"当前目录:"<<client->now_path<<endl;

    char request_path[kUserPathMaxLength]={0};
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

        FileSession tmp_session{};
        char send_packet_total[kHeaderSize + kBufferSize]={0};
        auto* send_packet_header = (header*)send_packet_total;
        auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
        tmp_session.state = kFileInit;
//        tmp_session.tick = clock();
        clock_gettime(CLOCK_MONOTONIC,&tmp_session.tick);
        tmp_session.file_fd = fd_tmp;
        tmp_session.file_ptr = fdopen(fd_tmp,"rb");
        srand(clock());
        tmp_session.sequence = random();
        tmp_session.session_id = session_id++;
        send_packet_header->file_proto.sequence = htonl(tmp_session.sequence);
        send_packet_header->file_proto.session_id =  htonl(tmp_session.session_id);
        file_list->push_back(tmp_session);
        strcpy(send_packet_data->file_response.file_path,receive_packet_data->file_request.file_path);
        send_packet_data->file_response.init_time = htonl(tmp_session.tick.tv_sec);
        send_packet_header->proto = kProtoFile;
        send_packet_header->file_proto.file_code=kFileResponse;
        send_packet_header->file_proto.data_length =  htonl(strlen(send_packet_data->file_response.file_path)
                                                      +sizeof(send_packet_data->file_response.init_time)) ;
        send(client->socket_fd, send_packet_total, kHeaderSize + ntohl(send_packet_header->file_proto.data_length), 0);

        cout<<"试图发送文件:"<<request_path<<endl;
        return 0;
    }
    else{
        cout<<"该路径文件不存在或文件不可用:"<<request_path<<endl;
        return -1;
    }
}

int ActionFileTranslating(FileSession* file_ss , ClientSession* client){
    char send_packet_total[kHeaderSize + kBufferSize]={0};
    auto* send_packet_header = (header*)send_packet_total;
    auto* send_packet_data = (data*)(send_packet_total + kHeaderSize);
    send_packet_header->file_proto.frame_transport_length= htonl(read(file_ss->file_fd,
                                                                send_packet_data->file_transport.file_data,
                                                                sizeof(send_packet_data->file_transport.file_data)));
    send_packet_header->proto=kProtoFile;
    send_packet_header->file_proto.file_code=kFileTransporting;
    send_packet_header->file_proto.session_id = htonl(file_ss->session_id);
    send_packet_header->file_proto.sequence = htonl(file_ss->sequence);
    send_packet_data->file_transport.CRC_32 = htonl(0); //先不写

    file_ss->sequence +=  ntohl(send_packet_header->file_proto.frame_transport_length);
    file_ss->state = kFileTransporting;
    if(ntohl(send_packet_header->file_proto.frame_transport_length) == 0){
        cout<<"文件传输完毕"<<endl;
        ActionFileEndSend(send_packet_total,client);
        return 0;
    }else if(ntohl(send_packet_header->file_proto.frame_transport_length) < 0){
        cout<<"文件读取失败"<<endl;
        return -1;
    }else{
        send_packet_header->file_proto.data_length = htonl(sizeof(send_packet_data->file_transport.CRC_32)
                                                     +ntohl(send_packet_header->file_proto.frame_transport_length));
        send(client->socket_fd, send_packet_total,
             kHeaderSize + ntohl(send_packet_header->file_proto.data_length) , 0);
//        fprintf(logger_fptr,"fd:%ld完成了一轮发送",clock()-file_ss->tick);
        return 0;
    }
}

int ActionFileAckReceived(const char* received_packet_total, ClientSession* client){
    auto* received_packet_header = (header*)received_packet_total;
    for(auto &file_ss_iter:*file_list){
        if(file_ss_iter.session_id == ntohl(received_packet_header->file_proto.session_id)){
            if(file_ss_iter.sequence == ntohl(received_packet_header->file_proto.sequence)){
                if(file_ss_iter.state == kFileInit){
                    cout<<"client "<<client->nickname<<"准备好接受文件传输！"<<endl;
                    ActionFileTranslating(&file_ss_iter,client);
                    return 0;
                }
                if(file_ss_iter.state == kFileTransporting){
                    ActionFileTranslating(&file_ss_iter,client);
                    return 0;
                }
            }
        }
    }
    cout<<"invalid File Ack packet"<<endl;
    return -1;
}

int ActionFileEndSend(const char* raw_packet, ClientSession* client){
    auto * send_packet_header = (header*)raw_packet;
    send_packet_header->file_proto.file_code = kFileEnd;
    send_packet_header->file_proto.data_length = htonl(0);
    send(client->socket_fd, raw_packet, kHeaderSize + ntohl(send_packet_header->file_proto.data_length), 0);
    for(auto file_ss_iter = file_list->begin(); file_ss_iter!=file_list->end();++file_ss_iter){
        if(file_ss_iter->session_id == ntohl(send_packet_header->file_proto.session_id)){
            close(file_ss_iter->file_fd);
            file_ss_iter->state = kFileEnd;
            file_list->erase(file_ss_iter);
            cout<<"文件指针已处理";
            return 0;
        }
    }

    cout<<"没有在列表里找到相应的文件"<<endl;
    return -1;
}

//region TimerSession实现代码
TimerSession::TimerSession(int timing_second,uint8_t the_timer_id,SocketFileDescriptor socket_fd)
        :timer_id_(the_timer_id),timing_second_(timing_second),socket_fd_(socket_fd),retry_count_(0),timer_state_(kTimerInit){
    clock_gettime(CLOCK_MONOTONIC,&init_tick_);
}
void TimerSession::set_timing_second(int timing_second) {
    timing_second_=timing_second;
}
int TimerSession::get_timing_second() const{
    return timing_second_;
}
void TimerSession::set_retry_max(int retry_max) {
    retry_max_ = retry_max;
}
bool TimerSession::TimeoutJustice() const {
    timespec tmp_time{};
    clock_gettime(CLOCK_MONOTONIC,&tmp_time);
    time_t tmp = tmp_time.tv_sec - init_tick_.tv_sec;
#if SERVER_DEBUG_LEVEL >2
    printf("(%d):%ld, ",timer_id_,tmp);
#endif
    fflush(stdout);
    return (tmp) >timing_second_;
}
bool TimerSession::TimerUpdate() {
    retry_count_+=1;
    if(retry_count_<retry_max_){
        timing_second_*=2;
        clock_gettime(CLOCK_MONOTONIC,&init_tick_);
        return true;
    } else{
        printf("过长时间没有回应(%d倍初始计时)，定时器%d自毁...\n",(1<<retry_max_)-1,timer_id_);
        TimerDisable();
        return false;
    }
}
bool TimerSession::TimerTrigger(){
    printf("Error:使用了父类的虚函数\n");
    return false;
}
bool TimerSession::TimerDisable() {
    if(timer_state_ == kTimerWorking){
        timer_state_ = kTimerDisable;
//        printf("定时器%d已关闭\n",timer_id_);
        fflush(stdout);
        return true;
    }
    return false;
}

State TimerSession::get_timer_state_(){
    return timer_state_;
}


TimerRetranslationSession::TimerRetranslationSession(int timing_second,uint8_t timer_id, SocketFileDescriptor client_fd,
                                                     const char *packet_cache,size_t packet_length):
        TimerSession(timing_second,timer_id,client_fd),client_fd_(client_fd),packet_length_(packet_length){
    memcpy(packet_cache_,packet_cache,packet_length);
    timer_state_ = kTimerWorking;
}
bool TimerRetranslationSession::TimerTrigger() {
    if(timer_state_ == kTimerWorking) {
//        TimerUpdate();
        if(!ErrorSimulator(kTimingErrorProb)){
#if SERVER_DEBUG_LEVEL>2
            printf("\ndebug:定时器(%d)重传成功\n",timer_id_);
#endif
            ssize_t x = send(client_fd_, packet_cache_, packet_length_, 0);
            return  x > 0;
        }
        else{
#if SERVER_DEBUG_LEVEL>2
            printf("\ndebug:定时器(%d)重传但是出现了差错了\n",timer_id_);
#endif
            return false;
        }
    }
    else
        return false;
}

TimerRemoveSession::TimerRemoveSession(int timing_second,uint8_t timer_id,SocketFileDescriptor socket_fd,bool(*function_point)()):
        TimerSession(timing_second,timer_id,socket_fd),trigger_void_function_(function_point){
    timer_state_ = kTimerWorking;
}
bool TimerRemoveSession::TimerTrigger() {
    if(timer_state_ == kTimerWorking){
//        TimerUpdate();
        return (*trigger_void_function_)();
    }
    else return false;
}
//endregion

bool TimerDisable(uint8_t timer_id,SocketFileDescriptor socket_fd){
    if(timer_id==0){
        return false;//特判 因为经常用
    }
    for(auto &iter :*timer_list){
        if(iter->timer_id_==timer_id){
            if(iter->socket_fd_ == socket_fd){
                return iter->TimerDisable();
            }
        }
    }
    return false;
}
