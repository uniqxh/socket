#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<signal.h>

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int main(int argc, char** argv){
	int cnt = 0;
	int n;
	if(argc > 1) n = atoi(argv[1]);
	else n = 100;
	while(n--){
		int sock;
		if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			sleep(4);
			perror("socket");
			exit(0);
		}
		struct sockaddr_in servaddr;
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons( 8888 );
		servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
			perror("connect");
			exit(0);
		}
		struct sockaddr_in localaddr;
		socklen_t len = sizeof(localaddr);
		if(getsockname(sock, (struct sockaddr*)&localaddr, &len) < 0){
			perror("getsockname");
			exit(0);
		}
		printf("ip=%s port=%d\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));
		printf("count = %d\n", ++cnt);
	}
	return 0;
}
