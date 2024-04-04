#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "npshell.h"

int main(int argc, char *argv[]){
    std::cout.setf(std::ios::unitbuf);

    clearenv();
    setenv("PATH", "bin:.", 1);
    printf("PATH=%s\n", getenv("PATH"));
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        exit(1);
    }
    
    int port = atoi(argv[1]);
    int msock, ssock;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    socklen_t clientLen;

    //建立socket
    if((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cerr << "socket error" << std::endl;
        exit(1);
    }
    //設定addr
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);//指定port
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);//有多個網卡的話任一網卡都可以接收

    int n, val = 1, len = sizeof(val);
    // n = setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &val, len);
	// if(n == -1){
	// 	perror("setsockopt(REUSEADDR)");
	// 	exit(1);
	// }
	// n = setsockopt(ssock, SOL_SOCKET, SO_REUSEPORT, &val, len);
	// if(n == -1){
	// 	perror("setsockopt(REUSEPORT)");
	// 	exit(1);
	// }

    //bind
    if(bind(msock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        std::cerr << "bind error" << std::endl;
        exit(1);
    }

    //listen
    n = listen(msock, 3);
	if(n == -1){
		perror("listen");
		exit(1);
	}
	// signal(SIGCHLD, handler);
    for(;;){
        dup2(0, 4);
        dup2(1, 5);
        dup2(2, 6);
        //accept
        ssock = accept(msock, (struct sockaddr*)&clientAddr, &clientLen);
        if(ssock < 0){
            std::cerr << "accept error" << std::endl;
            exit(1);
        }

        dup2(ssock, 0);
        dup2(ssock, 1);
        dup2(ssock, 2);

        npshell();

    }
    // int sockFd;
    // struct sockaddr_in serverAddr;

    // //建socket
    // if((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    //     std::cerr << "build sock stream error" << std::endl;
    //     exit(1);
    // }

    // //bind
    // memset((char *)&serverAddr, 0, sizeof(serverAddr));
    // // bzero(&serverAddr, sizeof(serverAddr));
    // serverAddr.sin_family = AF_INET;
    // serverAddr.sin_port = htons(port);//指定port
    // serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);//有多個網卡的話任一網卡都可以接收

    // if(bind(sockFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
    //     std::cerr << "server bind local address error" << std::endl;
    //     exit(1);
    // }

    // //listen
    // listen(sockFd, 5);

    // struct sockaddr_in clientAddr;
    // int addrlen = sizeof(clientAddr);
    // //accept
    // while(1){
    //     //因為會改動fd，所以先複製一份
    //     dup2(0, 4);
    //     dup2(1, 5);
    //     dup2(2, 6);
    //     int replySockFd;
    //     cout << "waiting for connection" << endl;
    //     //int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
    //     replySockFd = accept(sockFd, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen);//會回傳一個slave socket的fd
    //     if(replySockFd < 0){
    //         std::cerr << "server accept error" << std::endl;
    //         exit(1);
    //     }
    //     cout << "accept connection" << endl;
        
    //     std::cout << replySockFd << std::endl;
    //     //設定slave socket的fd給server socket
    //     dup2(replySockFd, 0);
    //     dup2(replySockFd, 1);
    //     dup2(replySockFd, 2);

    //     //執行npshell
    //     npshell();

    //     // close(replySockFd);

    //     //復原fd並close
    //     dup2(4, 0);
    //     dup2(5, 1);
    //     dup2(6, 2);
    //     close(4);
    //     close(5);
    //     close(6);
    //     close(replySockFd);
    // }

}
