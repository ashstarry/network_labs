/*
 * File:   receiver_main.cpp
 * Author: Yuanshun
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <string>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

#define INIT_SST 64
#define PKT_SIZE 1472
#define CONTENT_SIZE 1460
#define WAIT 0
#define SENT 1
#define ACKED 2
#define SLOW_START 0
#define CONGESTION_AVIODENCE 1
#define FAST_RECOVERY 2
#define RTT 20*100

struct sockaddr_in si_me, si_other;
int sockfd, slen;

void diep(string s) {
    perror(s.c_str());
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    slen = sizeof (si_other);

//    file description
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(sockfd, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


    /* Now receive data and send acknowledgements */
    //Data structure and constant initialization
    map<long long int, string> rcvPktBuf;
//    vector<string> rcvPktBuf;
    long long int latestPktInOrder = -1;
    long long int expectedPktIndex = 0;

//    store socket fd to buf
    char buf[PKT_SIZE];
    memset(buf, 0, sizeof(buf));
    FILE* fp = fopen(destinationFile, "wb");

    //不对
    struct sockaddr_in their_addr; //sender's address, get filled from recvfrom function(how does this work?)
    char buffer1[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &their_addr.sin_addr, buffer1, sizeof( buffer1));
    printf( "address:%s\n", buffer1 );

    int numbytes = 0;
    socklen_t their_addr_size = sizeof(their_addr);
    //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
//    int totalLen = 0;
    while(true) {
//        recvfrom return byte size of
        if ((numbytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&their_addr, &their_addr_size)) < 0 ) {
            diep("All Sockets Received");
        }
        cout << "recv bytes: " << numbytes << endl;
//        cout << "buf: " << buf << endl;

        //memcpy(&currSeqNum, buf, sizeof(unsigned long long int));
        //memcpy(&contentBuf, buf + 8, numbytes - 8);
        long int pktIndex;
        int contentLen;
//        char pktBuf[PKT_SIZE];
//        memset(pktBuf, 0, sizeof(pktBuf));

        //1.pktIndex = pkt index
        memcpy(&pktIndex, buf, sizeof(long int));
        cout << "pktIndex: " << pktIndex << endl;
        if(pktIndex == -1){
            cout << "FIN received" << endl;
            break;
        }
        cout << "expectedPktIndex: " << expectedPktIndex << endl;
//        2.pktContentLength
        memcpy(&contentLen, buf + 8, sizeof(int));
        cout << "contentLen: " << contentLen << endl;
//        3.pktBuf =  pkt content
//        cout << "unsigni: " << sizeof(unsigned long int) << endl;
//        cout << "unsigned lli: " << sizeof(unsigned long long int) << endl;

        char pktBuf[contentLen];
        memset(pktBuf, 0, sizeof(pktBuf));
        memcpy(&pktBuf, buf + 12, contentLen);
        string contentStr(pktBuf, contentLen);
//        totalLen = totalLen + contentStr.length();
        // cout << buf + 12 << endl;
        //cout << "recv bytes: " << numbytes << endl;
        //cout << "seqnum: " << pktIndex << endl;
        //cout << "content: " << contentStr << endl;
        cout << "contentStrLen: " << contentStr.length() << endl;
        //a packet that arrives too early, buffer it, and send dup acks
        // if (pktIndex > expectedPktIndex && (rcvPktBuf.find(pktIndex) == rcvPktBuf.end() || rcvPktBuf[pktIndex] == "")) {
//        if there is a new pkt,then put into map directly
        if (pktIndex > expectedPktIndex && rcvPktBuf[pktIndex].length() == 0) {
            cout << "New pkt, but not expected index" << endl;
            rcvPktBuf[pktIndex] = contentStr;
            //send back the expectedPktIndex - 1 to inform sender retransmit expectedPktIndex
            long long int dupAckIndex = expectedPktIndex - 1;
//            why use char[]?
            char ackChars[40];
            sprintf(ackChars, "%lld", dupAckIndex);
            string temp = ackChars;
            cout << "dupAckIndex: "  << dupAckIndex << endl;
            // memset(ackChars, 0, 12);
            // memcpy(ackChars, &dupAckIndex, 8);
            sendto(sockfd, ackChars, temp.length(), 0, (struct sockaddr *)&their_addr, their_addr_size);
//            sendto(sockfd, dupAckIndex, sizeof (long long int), 0, (struct sockaddr *)&their_addr, their_addr_size);
        }
            //a packet which is expected, deliver it, and go forward to deliver any buffered packets right behind it as well
        else if (pktIndex == expectedPktIndex) {
            cout << "Expected pkt arrives" << endl;
            // write to file and deliver
            fwrite(pktBuf, sizeof(char), contentLen, fp);
            fflush(fp);
            //release the buffered packets and write them all
            long long int deletePktIdx = pktIndex + 1;
            int deleteCnt = 0;
            // while (rcvPktBuf.find(deletePktIdx) != rcvPktBuf.end() &&      rcvPktBuf[deletePktIdx].length() != 0) {
            cout << "deletePktIdx: "  << deletePktIdx << endl;
            cout << "rcvPktBuf[deletePktIdx].length(): " << rcvPktBuf[deletePktIdx].length() << endl;
            while (rcvPktBuf[deletePktIdx].length() != 0) {
                fwrite(rcvPktBuf[deletePktIdx].c_str(), 1, rcvPktBuf[deletePktIdx].length(), fp);
                fflush(fp);
                // write to file and deliver
                std::map<long long int, string>::iterator it = rcvPktBuf.find(deletePktIdx);
                rcvPktBuf.erase(it);
                deleteCnt ++;
                deletePktIdx ++;
                cout << "deletePktIdx: "  << deletePktIdx << endl;
            }
            cout << "buffered packets: released and delivered!" << endl;
            cout << "delete count: "  << deleteCnt << endl;
            latestPktInOrder = deletePktIdx - 1;
            expectedPktIndex = deletePktIdx;

            char ackChars[40];
            sprintf(ackChars, "%lld", latestPktInOrder);
            string temp = ackChars;
            cout << "temp: " << temp << endl;
            // memset(ackChars, 0, 12);
            // memcpy(ackChars, &pktIndex, 8);
            sendto(sockfd, ackChars, temp.length(), 0, (struct sockaddr *)&their_addr, their_addr_size);
            cout << "The latest pkt Index of continuous pkt: "  << latestPktInOrder << endl;
//            cout << endl;
        }
            //an old packet, just resend the ack
        else if (pktIndex < expectedPktIndex) {
            char ackChars[40];
            sprintf(ackChars, "%lu", pktIndex);
            string temp = ackChars;
            cout << "ackChars: " << temp <<endl;
            // memset(ackChars, 0, 12);
            // memcpy(ackChars, &pktIndex, 8);
            sendto(sockfd, ackChars, temp.length(), 0, (struct sockaddr *)&their_addr, their_addr_size);
            cout << "Pkt index is stale, don't do anything: "  << pktIndex << endl;
//            cout << endl;
        }
    }

    fclose(fp);
    close(sockfd);
    printf("%s received.", destinationFile);
	cout << "test finish" << endl;
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
