#include<sys/types.h>
#include<sys/select.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define  maxcnt 1024
int main(){
	int ret;
	fd_set rfd;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	int listenfd;
	if((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("socket");
		exit(0);
	}
	printf("socket fd %d\n", listenfd);

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8888);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int on = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		perror("setsockopt");
		exit(0);
	}
	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		perror("bind");
		exit(0);
	}
	printf("listen max connetion %d\n", SOMAXCONN);
	if(listen(listenfd, SOMAXCONN) < 0){
		perror("listen");
		exit(0);
	}
	int fd[maxcnt] = {0};
	int i;
	int con_cnt = 0;
	int maxsock = listenfd;
	char buf[2048] = {0};
	int newfd;
	struct sockaddr_in clientsock;
	socklen_t len = sizeof(clientsock);
	while(1){
		FD_ZERO(&rfd);
		FD_SET(listenfd, &rfd);
		con_cnt = 0;
		for(i = 0;i<maxcnt;++i){
			if(fd[i] > 0){
				FD_SET(fd[i], &rfd);
				con_cnt ++;
			}
		}
		ret = select(maxsock + 1, &rfd, NULL, NULL, &tv);
		if(ret < 0){
			continue;
		}
		for(i = 0; i< maxcnt;++i){
			if(fd[i] > 0 && FD_ISSET(fd[i], &rfd)){
				ret = recv(fd[i], buf, sizeof(buf), 0);
				if(ret > 0){
					buf[ret] = '\0';
					printf("client[%d] send %s\n",i, buf);
				}else{
					con_cnt --;
					FD_CLR(fd[i], &rfd);
					close(fd[i]);
					fd[i] = 0;
				}
			}
		}
		if(FD_ISSET(listenfd, &rfd)){
			newfd = accept(listenfd, (struct sockaddr*)&clientsock, &len);
			if(newfd < 0){
				perror("accept");
				continue;
			}
			if(con_cnt < maxcnt){
				for(i = 0; i< maxcnt; ++i)
					if(fd[i] == 0){
						fd[i] = newfd;
						con_cnt ++;
						break;
					}
				printf("new accept %d\n", newfd);
				if(newfd > maxsock){
					maxsock = newfd;
				}
			}else{
				printf("max connection,exit client[%d]\n", newfd);
				send(newfd, "bye", 4, 0);
				close(newfd);
				continue;
			}
		}
	}
	for(i = 0; i<maxcnt;++i){
		if(fd[i] != 0){
			close(fd[i]);
		}
	}
	return 0;
}
