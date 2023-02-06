/*
 * File:   sender_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include<iostream>
#include<string>
#include <ctime>
#include <deque>
#include <cerrno>
using namespace std;

#define INIT_SST 256
#define PKT_SIZE 1472
#define CONTENT_SIZE 1460
#define WAIT 0
#define SENT 1
#define ACKED 2
#define SLOW_START 0
#define CONGESTION_AVIODENCE 1
#define FAST_RECOVERY 2
//#define RTT 10*1000
#define RTT 1000

struct Packet{
    struct timeval send_time; //the timestamp this package being sent
//    unsigned long int seqence_number; //sequence number
    long int seqence_number; //sequence number
    int status; //WAIT = 0 SENT = 1 ACKED = 2
    char content[PKT_SIZE];
}packet;

//receiver's address struct
struct sockaddr_in si_other;
int s;//socket
int slen; //receiver's address's length
float sst;
float cw;
int dupACK_count;


void diep(string s) {
    perror(s.c_str());
    exit(1);
}

void congestion_control(int current_status, string transition);
//void send_packet(struct Packet current_packet);

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    long int packet_number;
    long int cur_packet_index;
    long int cur_ack_seq_number;
    long int last_seq_number;
    //long int accu_seq_number;
    long int terminate_signal = -1;
    //long int offest = 0;
//    unsigned long int packet_number;
//    unsigned long int cur_packet_index;
//    unsigned int cur_ack_seq_number;
//    unsigned long int last_seq_number;
//    unsigned long int temp_seq_number;
    //int window_base;
    //int window_tail;
    int content_size;
    int last_content_size;
    int current_status;
    char send_buffer[PKT_SIZE];//whole packet available size
    char content_buffer[CONTENT_SIZE];//payload(real content)
    char ack_buffer[40];
    memset(ack_buffer,'\0',sizeof(ack_buffer));

    deque<Packet> conjestion_window;
    struct Packet current_packet;
    struct timeval timeout;

    struct sockaddr_in addr1;
//    char buffer1[INET_ADDRSTRLEN];
//    inet_ntop( AF_INET, &addr1.sin_addr, buffer1, sizeof( buffer1));
//    printf( "address:%s\n", buffer1 );
    socklen_t addr1_size = sizeof(addr1);

    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    /* Determine how many bytes to transfer */

    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }



//    addr_size = 16
//    cout << "addr_size: " << addr_size << endl;

    //variable initization
    //available payload 1472 bytes unsigned long int seq 8 bytes，content size 4 bytes 1472- 8 - 4=1460
    content_size = CONTENT_SIZE;
    cur_ack_seq_number = -1;
    last_seq_number = -1;
    sst = INIT_SST;
    cw = 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 2 * RTT;

    if(bytesToTransfer % content_size == 0){
        packet_number = (long int)bytesToTransfer / content_size;
        last_content_size = content_size;
    }
    else{
        packet_number = (long int)bytesToTransfer / content_size + 1;
        last_content_size = (long int)bytesToTransfer % content_size;
    }

    //下一个要读的包的index
    cur_packet_index = 0;
    current_status = SLOW_START;
    cout << "total_packet_number" << packet_number << endl;

    if (setsockopt(s, SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
        diep("setsockopt");
    }
    /* Send data and receive acknowledgements on s*/
    //main loop
    while(cur_packet_index < packet_number || !conjestion_window.empty()){//全发过了且window为空 结束传送
        //cout <<
        //28 + 8 + 1464 = 1500
        //conjestion window modification
        //1 shrink 搬到来了新包的时候
        //2 filling the tail
        //只管填充 不管终止条件
        //包全都发出了，则这里跳过
        cout << "-----loop while-----" << endl;
        while(cw < conjestion_window.size()){
            // cw < conjestion_window.size
            //出问题了 要把右边多余的都退掉重新来过 来了个超大的是在来新包的时候就shrink了
            conjestion_window.pop_back();
        }
        cout << "计划cw长度: " << cw << " sst: " << sst << " 目前cw size是： " << conjestion_window.size() << endl;
        while(cur_packet_index < packet_number && conjestion_window.size() < cw){
            //如果timeout cur_index不是最新数了
            //cur_index = last + 1

            //根据package index得到内容
            cout << "下一个将要发的包是: " << cur_packet_index << endl;

            int cur_content_size;//这里也要作为参数传给Rx
            int file_size;
            memset(content_buffer,'\0',sizeof(content_buffer));
            if(cur_packet_index != packet_number - 1){
                cur_content_size = content_size;
            }
            else{
                cur_content_size = last_content_size;
            }
            file_size = fread(content_buffer, sizeof(char), cur_content_size, fp);

            if(file_size < 0)
                diep("fread");

            memcpy(current_packet.content, &content_buffer, sizeof(char) * cur_content_size);
//            cout << "file content: "  << current_packet.content << endl;
//            cout << "sendfile content: "  << send_buffer << endl;
//            cout << "content_buffer: "  << content_buffer << endl;
            //gettimeofday(&current_packet.send_time, NULL);
            current_packet.seqence_number = cur_packet_index;
            current_packet.status = SENT;

            //send packet
//            memset(send_buffer,'\0',sizeof(send_buffer));
            memcpy(send_buffer, &(current_packet.seqence_number), sizeof(long int));
            cout << "current_packet.seqence_number: "  << current_packet.seqence_number << endl;
            memcpy(send_buffer + 8, &(cur_content_size), sizeof(int));
            cout << "cur_content_size: "  << cur_content_size << endl;
            memcpy(send_buffer + 12, current_packet.content, cur_content_size);
            //最后一个包的大小
            if(sendto(s, send_buffer, sizeof(send_buffer), 0, (sockaddr*)&si_other, slen) == -1){
                diep("send to");
            }

            conjestion_window.push_back(current_packet);
            cur_packet_index++;
        }

        //重置ack_buffer
        memset(ack_buffer,'\0',sizeof(ack_buffer));
        //receive ACK and change window setting
        int signal = recvfrom(s, ack_buffer, sizeof(ack_buffer), 0, (sockaddr *)&addr1, &addr1_size);
        //cout << "recvfrom signal " << signal << endl;
        //cout << " 收到ack_buffer: " << ack_buffer << endl;
        if(signal == -1) {
            //timeout

            char buffer1[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &addr1.sin_addr, buffer1, sizeof( buffer1));
            //printf( "address:%s\n", buffer1 );

            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                diep("ACK Receive Error");
            }
            //重传base package
            cout << conjestion_window.front().seqence_number << " package time out happen" << endl;
            //这个时候，下一个要发的包是cw里面的第一个包
            cur_packet_index = conjestion_window.front().seqence_number;
            conjestion_window.clear();
            //文件读取指针指向新的将发包
            fseek(fp, 1460 * cur_packet_index, SEEK_SET);
            //            Packet resend_pkt = conjestion_window.front();
//
//            memset(send_buffer,'\0',sizeof(send_buffer));
//            memcpy(send_buffer, &(resend_pkt.seqence_number), sizeof(unsigned long int));
//            memcpy(send_buffer + 8, resend_pkt.content, strlen(resend_pkt.content));
//            //最后一个包的大小
//            if(sendto(s, send_buffer, sizeof(send_buffer), 0, (sockaddr*)&si_other, slen) == -1){
//                diep("send to");
//            }

            //usleep(100000);
            congestion_control(current_status, "timeout");
        }
        else if(signal >= 0){
            //正常，拆包，得到seq_number
            sscanf(ack_buffer, "%ld", &cur_ack_seq_number);
            cout << "正常" << endl;
            cout << "cur_ack_seq_number: " << cur_ack_seq_number << endl;
            cout << "last_seq_number: " << last_seq_number << endl;
            //应该是0
            // last_seq_number = -1
            if(cur_ack_seq_number == last_seq_number){
                //duplicate
                dupACK_count++;
                if(current_status == FAST_RECOVERY){
                    cw = cw + 1;
                }
                else{
                    if(dupACK_count == 3){
                        congestion_control(current_status, "3dupACK");
                    }
                }
            }
                //
            else if(cur_ack_seq_number < last_seq_number){
                cout << "stale pkt, no action" << endl;
                //stale packet
                continue;
            }
            else{
                //conjestion window shrink
                //回来的ACK是window base，安顺序来的
                if(cur_ack_seq_number == last_seq_number + 1){
                    conjestion_window.pop_front();
                    last_seq_number = cur_ack_seq_number;
                    congestion_control(current_status, "newACK");
                }
                else{
                    //cur_ack_seq_number >> last_seq_number
                    //跳着来了个很后面的ack，记录要送多少次
                    //shrink 小于当前的包退出来
                    //如果来的数字在conjestion window里
                    //cur_index 下一个要发的包不变，继续增加
                    if(cur_ack_seq_number <= conjestion_window.back().seqence_number){
                        while(!conjestion_window.empty() && (conjestion_window.front().seqence_number <= cur_ack_seq_number)){
                            last_seq_number = conjestion_window.front().seqence_number;
                            conjestion_window.pop_front();
                            congestion_control(current_status, "newACK");
                        }
                        last_seq_number = cur_ack_seq_number;
                    }
                        //如果来的数字比window tail还要大
                        //cur_index 下一个要发的包要变大了
                    else if(cur_ack_seq_number > conjestion_window.back().seqence_number){
                        conjestion_window.clear();
                        for(int i = last_seq_number+1; i <= cur_ack_seq_number; i++){
                            congestion_control(current_status, "newACK");
                        }
                        last_seq_number = cur_ack_seq_number;
                        cur_packet_index = cur_ack_seq_number + 1;
                        fseek(fp, 1460 * cur_packet_index, SEEK_SET);
                    }
                    //如果来了个很后面的，按顺序一个一个送
//                    while(accu_seq_number <= cur_ack_seq_number){
//                        //cout << 2 << endl;
//                        congestion_control(current_status, "newACK");
//                        accu_seq_number += 1;
//                    }

//                    while(last_seq_number <= cur_ack_seq_number){
//                        last_seq_number++;
//                        conjestion_window.pop_front();
//                        congestion_control(current_status, "newACK");
//                    }

                }

                //cout << "last_seq_number: " << last_seq_number << endl;
            }
        }
        cout << "----- endloop -----" << endl;
    }



    char fin_buffer[40];
    while(true){
        memset(send_buffer,'\0',sizeof(send_buffer));
        memcpy(send_buffer, &(terminate_signal), sizeof(long int));
        if(sendto(s, send_buffer, sizeof(send_buffer), 0, (sockaddr*)&si_other, slen) == -1){
            diep("send to");
        }

        if (setsockopt(s, SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
            diep("setsockopt");
        }

        int signal = recvfrom(s, fin_buffer, sizeof(fin_buffer), 0, (sockaddr *)&addr1, &addr1_size);
        if(signal == -1){
            diep("handshake");
        }
        sscanf(fin_buffer, "%ld", &cur_ack_seq_number);
        if (cur_ack_seq_number == -1) {
            cout << "Receive the FIN_ACK" << endl;
            break;
        }
    }

    printf("Closing the socket\n");
    close(s);
    fclose(fp);
    return;

}

void congestion_control(int current_status, string transition){
    if(current_status == SLOW_START){
        if("timeout" == transition){
            sst = cw / 2;
            cw = 1;
            dupACK_count = 0;
            current_status = SLOW_START;

        }
        else if("3dupACK" == transition){
            sst = cw / 2;
            cw = sst + 3;
            current_status = FAST_RECOVERY;
            //hole packet?
        }
        else if("newACK" == transition){
            cw += 3;
            dupACK_count = 0;
            if(cw >= sst){
                current_status = CONGESTION_AVIODENCE;
            }
        }

    }
    else if(current_status == CONGESTION_AVIODENCE){
        if("timeout" == transition){
            sst = cw / 2;
            cw = 1;
            dupACK_count = 0;
            current_status = SLOW_START;
        }
        else if("3dupACK" == transition){
            sst = cw / 2;
            cw = sst + 3;
            current_status = FAST_RECOVERY;
            //hole packet?
        }
        else if("newACK" == transition){
            cw = cw + 1 / floor(cw);
            dupACK_count = 0;
        }
    }
    else if(current_status == FAST_RECOVERY){
        if("timeout" == transition){
            sst = cw / 2;
            cw = 1;
            dupACK_count = 0;
            current_status = SLOW_START;
        }
        else if("newACK" == transition){
            cw = sst;
            dupACK_count = 0;
            current_status = CONGESTION_AVIODENCE;
        }
    }

    if(cw < 1)
        cw = 1;
    if(sst < 1)
        sst = 1;
}

int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
	cout << "test" << endl;

    return (EXIT_SUCCESS);
}
