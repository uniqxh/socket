#include<sys/types.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/socket.h>
#include<errno.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define maxevent 10000
void getFileType(char* fileType, char* fileName){
	if(strstr(fileName, ".html")) strcpy(fileType, "text/html");
	else if(strstr(fileName, ".gif")) strcpy(fileType, "image/gif");
	else if(strstr(fileName, ".jpg")) strcpy(fileType, "image/jpeg");
	else if(strstr(fileName, ".png")) strcpy(fileType, "image/png");
	else if(strstr(fileName, ".ico")) strcpy(fileType, "image/x-icon");
	else strcpy(fileType, "text/plain");
}
void toUpper(char* s){
	char* d = s;
	while(*d != '\0'){
		*d = toupper(*d);
		++d;
	}
}
int writen(int fd,void* buf, size_t n){
	size_t l = n;
	size_t w;
	char* bf = buf;
	while(l > 0){
		if((w = write(fd, bf, l)) <= 0){
			if(errno == EINTR) w = 0;
			else return -1;
		}
		l -= w;
		bf += w;
	}
	return n;
}
int setnonblocking(int fd){
	if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) == -1){
		return -1;
	}
	return 0;
}
int responseSuccess(int fd, int len, char* u){
	char fileType[20];
	getFileType(fileType, u);
	char buf[2048] = {0};
	sprintf(buf, "HTTP/1.1 200 OK\r\n");
	sprintf(buf, "%sServer: myTiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, len);
	sprintf(buf, "%sContent-type: %s;charset=utf-8\r\n\r\n",buf, fileType);
	writen(fd, buf, strlen(buf));
	printf("[%s]head length: %lu, body length: %d\n", u, strlen(buf), len);
	return 0;
}
int responseFail(int fd){
	char buf[2048] = {0};
	sprintf(buf, "HTTP/1.1 404 NOT found\r\n");
	sprintf(buf, "%sServer: myTiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: 0\r\n", buf);
	sprintf(buf, "%sContent-type: text/html;charset=utf-8\r\n\r\n",buf);
	writen(fd, buf, strlen(buf));
	return 0;
}
int compute(int fd, char* u, char* d){
	int a=0,b=0;
	char data[1024]={0};
	if(d != NULL) sscanf(d, "a=%d&b=%d", &a, &b);
	toUpper(u);
	if(strcmp(u, "/ADD") == 0) sprintf(data, "%d + %d = %d\n", a, b, a+b);
	else if(strcmp(u, "/SUB") == 0) sprintf(data, "%d - %d = %d\n", a, b, a-b);
	else if(strcmp(u, "/MUL") == 0) sprintf(data, "%d * %d = %d\n", a, b, a*b);
	else if(strcmp(u, "/DIV") == 0) sprintf(data, "%d / %d = %d\n", a, b, a/b);
	else{
		responseFail(fd);
		return -1;
	}
	responseSuccess(fd, strlen(data), u);
	writen(fd, data, strlen(data));
	return 0;
}
int writeWeb(int fd, char* u, char* d){
	struct stat sb;
	size_t len = 0;
	if(stat(u, &sb) >= 0) len = sb.st_size;
	int ffd = open(u, O_RDONLY, 0);
	if(ffd < 0){
		compute(fd, u, d);
		return -1;
	}
	responseSuccess(fd, len, u);
	char* mp = mmap(0, len, PROT_READ, MAP_PRIVATE, ffd, 0);
	close(ffd);
	writen(fd, mp, len);
	munmap(mp, len);
	return 0;
}
int parseUrl(int fd, char* m, char* u, char* v, char* d){
	if(strcmp(m,"GET") == 0){
		if(strcmp(u, "/") == 0){
			sprintf(u, "%sindex.html", u);
			sscanf(u, "/%s", u);
		}
		printf("GET [%s]\n", u);
		writeWeb(fd, u, d);
	}else if(strcmp(m, "POST") == 0){
		printf("POST [%s]\n", u);
		compute(fd, u, d);
	}else{
		printf("unknown method\n");
		responseFail(fd);
	}
	return 0;
}
int main(){
	int sfd,fd;
	struct epoll_event event;
	struct epoll_event* ev;
	struct sockaddr_in servaddr, clientaddr;
	socklen_t len = sizeof(clientaddr);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8888);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if((sfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("socket");
		exit(0);
	}
	int on = 1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		perror("setsockopt");
		exit(0);
	}
	if(setnonblocking(sfd) < 0){
		perror("setnonblocking");
		exit(0);
	}
	if(bind(sfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		perror("bind");
		exit(0);
	}
	if(listen(sfd, SOMAXCONN) < 0){
		perror("listen");
		exit(0);
	}

	ev = calloc(maxevent, sizeof(event));
	int efd = epoll_create1(0);
	if(efd < 0){
		perror("epoll_create");
		exit(0);
	}
	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0){
		perror("epoll_ctl");
		close(efd);
		exit(0);
	}
	int cfd = 1;
	int cnt = 0;
	int newfd;
	int i,ret;
	char buf[1024] = {0};
	while(1){
		cnt = epoll_wait(efd, ev, maxevent, 0);
		if(cnt < 0){
			perror("epoll_wait");
			continue;
		}
		for(i = 0; i<cnt; ++i){
			fd = ev[i].data.fd;
			if(fd == sfd){
				newfd = accept(sfd, (struct sockaddr*)&clientaddr, &len);
				if(newfd < 0){
					perror("accept");
					continue;
				}
				if(cfd >= maxevent){
					fprintf(stderr, "too many connection, more than %d\n", maxevent);
					close(newfd);
					continue;
				}
				if(setnonblocking(sfd) < 0){
					perror("setnonblocking");
					exit(0);
				}
				printf("new accept client %d\n", newfd);
				event.data.fd = newfd;
				event.events = EPOLLIN | EPOLLET;
				if(epoll_ctl(efd, EPOLL_CTL_ADD, newfd, &event) < 0){
					perror("epoll_ctl");
					continue;
				}
				cfd ++;
				continue;
			}
			memset(buf, 0, sizeof(buf));
			ret = read(fd, buf, sizeof(buf));
			if(ret <= 0){
				close(fd);
				epoll_ctl(efd, EPOLL_CTL_DEL, fd, &event);
				cfd --;
			}else{
				char m[10], u[100], v[20], d[1024], *p;
				sscanf(buf,"%s %s %s", m, u, v);
				if(strcmp(m, "POST") == 0){
					p = strstr(buf, "\n\r\n");
					if(p == NULL) p = buf + strlen(buf);
					else p += 3;
				}else if(strcmp(m, "GET") == 0){
					p = strstr(u, "?");
					if(p == NULL) p = u + strlen(u);
					else {
						*p = '\0';
						p += 1;
					}
				}
				parseUrl(fd, m, u, v, p);
			}
		}
	}
	return 0;
}
