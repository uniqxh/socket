#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<poll.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>

int main(){
	int cnt = 0;
	signal(SIGPIPE, SIG_IGN);
	int listenfd;
	if((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("sock");
		exit(0);
	}
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
	if(listen(listenfd, SOMAXCONN) < 0){
		perror("listen");
		exit(0);
	}
	struct sockaddr_in peeraddr;
	socklen_t len = sizeof(peeraddr);
	int conn;
	int i;
	struct pollfd client[2048];
	int maxi = 0;
	for(i = 0;i < 2048; ++i) client[i].fd = -1;
	int nready;
	client[0].fd = listenfd;
	client[0].events = POLLIN;
	while(1){
		nready = poll(client, maxi + 1, -1);
		if(nready == -1){
			if(errno == EINTR) continue;
			perror("poll");
			exit(0);
		}
		if(nready == 0) continue;
		if(client[0].revents & POLLIN){
			conn = accept(listenfd, (struct sockaddr*)&peeraddr, &len);
			if(conn == -1){
				perror("accept");
				exit(0);
			}
			for(i = 0;i<2048;++i){
				if(client[i].fd < 0){
					client[i].fd = conn;
					if(i > maxi) maxi = i;
					break;
				}
			}
			if(i == 2048){
				fprintf(stderr, "too many clients\n");
				exit(0);
			}
			printf("count = %d\n", ++ cnt);
			printf("receive connect ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
			client[i].events = POLLIN;
			if(--nready <= 0) continue;
		}
		for(i = 1; i<=maxi;++i){
			conn = client[i].fd;
			if(conn == -1) continue;
			if(client[i].revents & POLLIN){
				char recvbuf[1024] = {0};
				int ret = read(conn, recvbuf, 1024);
				if(ret == -1) {
					perror("readline");
					exit(0);
				}
				else if(ret == 0){
					printf("client close\n");
					client[i].fd = -1;
					close(conn);
				}
				fputs(recvbuf, stdout);
				write(conn, recvbuf, strlen(recvbuf));
				if(--nready <=0) break;
			}
		}
	}
	return 0;
}
