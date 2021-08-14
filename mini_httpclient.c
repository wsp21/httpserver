#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/socket.h>
#include<ctype.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#define SERVER_PORT 80
#define SERVER_IP "127.0.0.1"

int main(int argc,char *argv[]){
	
	int sockfd,n;
	char *message;
	struct sockaddr_in servaddr;
	char buf[64];

	if(argc!=2){
		fputs("Usage: ./echo_client message \n",stderr);
		exit(1);
	}

	message=argv[1];
	printf("message: %s \n",message);

	sockfd = socket(AF_INET,SOCK_STREAM,0);

	memset(&servaddr,'\0',sizeof(struct sockaddr_in));

	servaddr.sin_family=AF_INET;
	inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);
	servaddr.sin_port = htons(SERVER_PORT);
	
	connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

	write(sockfd,message,strlen(message));
	
	n= read(sockfd,buf,sizeof(buf)-1);
		
	if(n>0){
		buf[n]='\0';
		printf("recive: %s \n",buf);	
	}else{
		printf("error!\n");
	}

	printf("finished\n");
	close(sockfd);

	return 0;
}
