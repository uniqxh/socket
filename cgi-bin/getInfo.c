#include<stdio.h>
#include<stdlib.h>
#include<string.h>
void response(char *p){
	printf("Content-length: %lu\r\n", strlen(p));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s", p);
	fflush(stdout);
}
int main(){
	char content[1024], *buf, *p;
	buf=p=NULL;
	if((buf = getenv("QUERY_STRING")) != NULL){
		p = strchr(buf, '&');
		*p = '\0';
	}
	int flag = 0;
	if(buf == NULL || strlen(buf) <= 0 || *buf == '\0'){
		sprintf(content, "用户名%s无效", buf);
		flag = 1;
	}
	if(p == NULL || strlen(p+1) <= 0 || *(p+1) == '\0'){
		sprintf(content, "%s 密码%s无效\r\n", content, p+1);
		flag = 1;
	}
	if(!flag){
		sprintf(content, "Welcome to %s\n\t%s\n\t%s\r\n\r\n", buf, buf, p + 1);
	}
	response(content);
	exit(0);
}
