/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;
//#define PORT "80"  // the port users will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define BACKLOG 10  // how many pending connections queue will hold
const char* HEAD = "HTTP/1.0 200 OK";
const char* NEW_LINE="\r\n";
const char* CONTENT_START = "\r\n\r\n";
const char* CONTENT_LENGTH = "Content-Length: %d";
const char* CONTENT_TYPE = "Content-Type: text/html";

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
	printf("%d",s);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	string PORT;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv, numbytes;
    //int nextSlash = -1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
	
	if(argc != 2){
		exit(0);	
	}
	PORT = argv[1];
    if ((rv = getaddrinfo(NULL, PORT.data(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            char buf[1024] = "qwertyuiop";
            //char* buf = (char*)malloc(1024 * sizeof(char));
//            send(new_fd, buf, 20, 0);
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
//            printf("server: received\n %s\n",buf);
            std::string str = "";
            bool isContext = 0;
            for(int i = 0; i < numbytes; i++){
                if(buf[i] == 'G'){
                    isContext = 1;
                }
                if(isContext) str += buf[i];
//                {
//                    isContext != isContext;
//                    continuee; }

            }
            printf("server: received\n %s\n",str.c_str());
            char request_buf[MAXDATASIZE];
            std::string request(request_buf);
            int getIndex = str.find_first_of("GET ");
            if(getIndex != 0){
                request = "HTTP/1.1 400 Bad Request\r\n\r\n";
                if (send(new_fd, request.data(), request.size(), 0) == -1)
                    perror("send");
                exit(1);
            }
            str = str.substr(4);
            printf("server: without get is \n %s\n",str.c_str());
            int temp2 = str.find_first_of(" HTTP");
            if (temp2 == request.npos) {
                request = "HTTP/1.1 400 Bad Request\r\n\r\n";
                if (send(new_fd, request.data(), request.size(), 0) == -1)
                    perror("send");
                exit(1);
            }


            str = str.substr(1, temp2-1);
            str = "./" + str;
//            str = str.substr(5, str.size());
//            cout<<"str: "+ str<<endl;
//            nextSlash = str.find_first_of("/");
//            printf("nextSlash:%hd\n",nextSlash);
//            printf("server: received\n %s\n",str.c_str());
//            str = str.substr(0, nextSlash - 5);
//            printf("server: received\n %s\n",str.c_str());
//            str = "./" + str;
            printf("server: path is \n %s\n",str.c_str());

//          read file
            int is_ok = EXIT_FAILURE;
//            int str_len = str.size();
            const char* fname = str.c_str(); // or tmpnam(NULL);
//            char * fn_new = (char*)malloc(str_len);
//            for(int i = 0; i < str_len-1; ++i)
//                fn_new[i] = fname[i];
//            fn_new[str_len -1] = '\0';
//            printf("%s##\n",fname);
//            for(int i = 0; i < str.size(); ++i) printf("%c\n" ,fn_new[i]);
            FILE* fp = fopen(fname, "r");
            if(!fp) {
                perror("File opening failed");
                return is_ok;
            }
//            fputs("Open File Scuccessfully!\n", fp);
            rewind(fp);

            int c; // note: int, not char, required to handle EOF
            int length_count = 0;
            while ((c = fgetc(fp)) != EOF) { // standard C I/O file reading loop
                putchar(c);
                length_count += 1;
            }

            if (ferror(fp)) {
                puts("I/O error when reading");
            } else if (feof(fp)) {
                puts("End of file reached successfully");
                is_ok = EXIT_SUCCESS;
            }
//            begin transaction
            char content[MAXDATASIZE];
            int sent = 0;
            int n;
            //int firstRead = 1;
            //int iCOunt = 0;
//            memset(content, '\0',MAXDATASIZE);
            char cur_head[100];
            cur_head[0] = '\0';
            strcat(cur_head, HEAD);
            strcat(cur_head, NEW_LINE);
            strcat(cur_head, CONTENT_TYPE);
            strcat(cur_head, NEW_LINE);
            sprintf(cur_head + strlen(cur_head), CONTENT_LENGTH, length_count);
            strcat(cur_head, CONTENT_START);

            if (send(new_fd, cur_head, strlen(cur_head), 0) == -1){
                perror("send1");
                exit(1);
            }
            rewind(fp);
            while(1){
                memset(content, 0,MAXDATASIZE);
                n = fread(content, sizeof (char), MAXDATASIZE, fp);
                n = send(new_fd, content, n, 0);
                if (n == -1){
                    printf("error occurred and the packets have not been sent completely");
                    break;
                }
//                if (firstRead) {
//                    n = fread(content + strlen(HEAD), sizeof (char), MAXDATASIZE - strlen(HEAD), fp);
//                    n = send(new_fd, content, n + strlen(HEAD), 0);
//                    if (n == -1){
//                        printf("error occurred and the packets have not been sent completely");
//                        break;
//                    }
//                    firstRead = 0;
//                } else {
//                    n = fread(content, sizeof (char), MAXDATASIZE, fp);
//                    n = send(new_fd, content, n, 0);
//                    if (n == -1){
//                        printf("error occurred and the packets have not been sent completely");
//                        break;
//                    }
//                }
                //printf("sent:%d\n",sent);
                if (n == 0) {
                    break;
                }

                sent += n;
                printf("sent is %hd\r\n", sent);
            }

            printf("server: file transaction sucessfully\r\n");
//            end transaction
            fclose(fp);
//            remove(fname);
            return is_ok;
            //          end write

            close(new_fd);
            //free(buf);
            exit(0);
        }


        //close(new_fd);  // parent doesn't need this
    }

    return 0;
}

