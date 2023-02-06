/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <sys/socket.h>

#include <arpa/inet.h>
#include <string>
#include <iostream>
#include<unistd.h>
#include<fcntl.h>
#include <sys/time.h>

using namespace std;
#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 350 // max number of bytes we can get at once


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
   	 return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* mysubstr(char* arr, int begin, int len)
{
    char* res = new char[len + 1];
    for (int i = 0; i < len; i++)
        res[i] = *(arr + begin + i);
    res[len] = 0;
    return res;
}

int main(int argc, char *argv[])
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    //char s[INET6_ADDRSTRLEN];

    //const parsed URL conponent setting
    string URL;
    string http_vertify("http://");
    string PATH, HOST, TAR_PORT;
    int port_end = -1;
    int host_end = -1;
    string request_body;
    const char* request_array;
    int size_recv , total_size= 0;
    char chunk[MAXDATASIZE];
    if (argc != 2) {
    	fprintf(stderr,"usage: client hostname\n");
    	exit(1);
    }
    

    //split url into host, port and path
    URL = argv[1];
    cout<<URL<<endl;
    if(URL.compare(0, http_vertify.size(), http_vertify)){
   	 fprintf(stderr,"This client only serve HTTP protocol\n");
    	exit(1);
    }

    URL = URL.substr(7, URL.size());
    cout<<"URL: "+URL<<endl;
    port_end = URL.find_first_of("/");

    
    if(port_end == -1){
   	 fprintf(stderr,"Invaild Path\n");
    	exit(1);
    }
    
    host_end = (URL.find_first_of(':') == -1 || URL.find_first_of(':') >= port_end)? port_end: URL.find_first_of(':');

    if(host_end == port_end){
   	 HOST = URL.substr(0, host_end);
   	 TAR_PORT = "80";
    }
    else{
   	 HOST = URL.substr(0, host_end);
   	 TAR_PORT = URL.substr(host_end+1, port_end-host_end-1);   	 
    }
    PATH = URL.substr(port_end, URL.size()-port_end);
    cout<<"HOST: "<<HOST<<endl;
    cout<<"TAR_PORT: "<<TAR_PORT<<endl;

    cout<<"PATH: "<<PATH<<endl;
    cout<<"\n"<<endl;
    

    request_body = "GET " + PATH + " HTTP/1.1\r\n"
          	+ "User-Agent: Wget/1.12(linux-gnu)\r\n"
          	+ "Host: " + HOST + ":" + TAR_PORT + "\r\n"
          	+ "Connection: Keep-Alive\r\n\r\n";

	//request_array = "GET /index.html HTTP/1.1\r\nUser-Agent: Wget/1.12(linux-gnu)\r\nHost: 8.8.8.8:80\r\nConnection: Keep-Alive\r\n\r\n";
    request_array = request_body.data();
    cout<<request_array<<endl;
    cout<<strlen(request_array)<<endl;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(HOST.data(), TAR_PORT.data(), &hints, &servinfo)) != 0) {
   	 fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
   	 return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
   	 if ((sockfd = socket(p->ai_family, p->ai_socktype,
   			 p->ai_protocol)) == -1) {
   		 perror("client: socket");
   		 continue;
   	 }

   	 if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
   		 close(sockfd);
   		 perror("client: connect");
   		 continue;
   	 }

   	 break;
    }

    //Send some data
    if( send(sockfd, request_array, strlen(request_array), 0) < 0)
    {
   	 perror("socket connected, but transaction failed");
   	 exit(1);
    }
    
    cout<<"Request Send"<<endl;


    //ofstream outfile;
	//outfile.open("output", ios::binary);

    //Receive a reply from the server

	FILE* output;
	if((output=fopen("./output","wb"))==NULL){
		perror("client:creating output file failed");
		exit(1);
	}
	
	/**
	if( size_recv = recv(sockfd, chunk , 1000 , 0) < 0)
	{
		puts("recv failed");
		return 0;
	}
	puts("Reply received\n");
	puts(chunk);
	fwrite(chunk, strlen(chunk),1,output);
	**/
	
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;	
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    	int head = 1;
	while(1)
    {
   	 memset(chunk ,0 , MAXDATASIZE);    //clear the variable
	size_recv = recv(sockfd, chunk, MAXDATASIZE, MSG_WAITALL);
   	 if(size_recv == -1)
   	 { 	
		//error break;
		break;
   	 }
	 else if(size_recv == 0){
		//safe break, no byte in the chunk now
		break;	
	}
   	 else
   	 {	
		if(head){
			string first_body(chunk);
			int body_begin = first_body.find("\r\n\r\n") + 4;
			first_body = first_body.substr(body_begin, strlen(chunk)-body_begin);
			fwrite(first_body.data(),strlen(first_body.data()),1,output);	
			printf("%s" , first_body.data());
			head = 0;
			total_size += strlen(first_body.data());
		}
		else{
	   		total_size += size_recv;
			fwrite(chunk,size_recv,1,output);		
			printf("%s" , chunk);
			printf("----");
		}
   	 }
    }




    cout<<total_size<<" bytes response body received"<<endl;

	fclose(output);
    close(sockfd);

    return 0;
}
