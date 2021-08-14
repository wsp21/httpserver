#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_PORT 80
static int debug = 1;
void do_http_response(int client_sock);

//异常处理
void bad_request(int client_sock);//400客户端发送失败
void unimplement(int client_sock);//501服务器方法没实现
void not_found(int client_sock);//404文件不存在

//行读取http请求
int get_line(int sock, char *buf, int size)
{
	int count = 0;
	char ch = '\0';
	int len = 0;
	while ((count < size - 1) && ch != '\n')
	{
		len = read(sock, &ch, 1);
		if (len == 1)
		{
			if (ch == '\r')
			{
				continue;
			}
			else if (ch == '\n')
			{
				buf[count] = '\0';
				break;
			}
			buf[count] = ch;
			count++;
		}
		else if (len == -1)
		{
			perror("read failed!\n");
			break;
		}
		else
		{ //客户端socket关闭，read返回0
			fprintf(stderr, "client close!\n");
			break;
		}
	}
	return count;
}

void do_http_request(int client_sock)
{
	//读取客户端发送http请求

	//读取请求行
	int len = 0;
	char client_ip[64];
	char buf[256];
	char method[16];
	char url[256];
	char path[512];
	struct stat st;

	len = get_line(client_sock, buf, sizeof(buf));

	if (len > 0)
	{
		int i = 0, j = 0;
		while (!isspace(buf[j]) && (i < sizeof(method) - 1))
		{
			method[i] = buf[j];
			i++;
			j++;
		}
		method[i] = '\0';

		//判断合法
		if (strncasecmp(method, "GET", i) == 0)
		{
			printf("request = %s\n", method);

			//获取url
			while (isspace(buf[++j]))
				;
			i = 0;
			while (!isspace(buf[j]) && (i < sizeof(url) - 1))
			{
				url[i] = buf[j];
				i++;
				j++;
			}
			url[i] = '\0';
			printf("url: %s\n", url);

			sprintf(path,"./html_docs%s",url);
			if(path[strlen(path)-1]=='/'){
				strcat(path,"index.html");
			}
			printf("path:%s\n",path);

			do
			{
				len = get_line(client_sock, buf, sizeof(buf));
				printf("read line: %s\n", buf);
			} while (len > 0);

			if(stat(path,&st)){//文件不存在或异常
				not_found(client_sock);
			}else{
				if(S_ISDIR(st.st_mode)){
					strcat(path,"./index.html");
				}

				printf("final path:%s\n",path);
				
				do_http_response(client_sock);
			}
			
		}
		else
		{
			printf("other request = %s\n", method);
			do
			{
				len = get_line(client_sock, buf, sizeof(buf));
				printf("read line: %s\n", buf);
			} while (len > 0);

			unimplement(client_sock);
		}
	}
	else
	{ //出错处理
		bad_request(client_sock);
	}
}

void do_http_response(int client_sock)
{
	const char *main_header = "HTTP 1.0 200 OK\r\nServer: WSP Server\r\nContent-Type:test/html\r\nConnection:Close\r\n";
	const char *main_body = "\
<html>\n\
<head>\n\
<meta charset=\"UTF-8\">\n\
<title>管理员登录主页</title>\n\
</head>\n\
<body>\n\
	<form action=\"login1\" method=\"post\">\n\
		<table border=\"0\" align=\"center\">\n\
			<tr>\n\
				<td align=\"center\" colspan=\"3\"><h4>管理员身份验证</h4></td>\n\
			</tr>\n\
			<tr>\n\
				<td>管理员id：</td>\n\
				<td><input type=\"text\" name=\"id\" value="
							"></td>\n\
			</tr>\n\
			<tr>\n\
				<td>密码：</td>\n\
				<td><input type=\"password\" name=\"password\" value="
							"></td>\n\
			</tr>\n\
			<tr align=\"center\">\n\
				<td align=\"center\" colspan=\"3\"><input type=\"button\" value=\"确定\"\n\
					onclick=\"f()\" />&nbsp; <input type=\"reset\" value=\"取消\" /></td>\n\
			</tr>\n\
		</table>\n\
	</form>\n\
</body>\n\
</html>";
	char send_buf[64];
	int body_len = strlen(main_body);


	//http报文头部
	/*//int len = write(client_sock, main_header, strlen(main_header));

	if (debug)
		fprintf(stdout, "...do_http_response...\n");
	if (debug)
		fprintf(stdout, "write[%d]: %s", len, main_header);
	//len = snprintf(send_buf, 64, "Content_Length:%d\r\n\r\n", body_len);
	//len = write(client_sock, send_buf, len);
	if (debug)
		fprintf(stdout, "write[%d]: %s", len, send_buf);
	*/
	int len = write(client_sock, main_body, body_len);
	if (debug)
		fprintf(stdout, "write[%d]:\n%s\n", len, main_body);
}

void bad_request(int client_sock){
	const char *reply="\
<html>\n\
<head>\n\
<meta charset=\"UTF-8\">\n\
<title>BAD REQUEST</title>\n\
</head>\n\
<body>\n\
	<p>Your browser send a bad request!\n\
</body>\n\
</html>";
	int len=write(client_sock,reply,strlen(reply));
	if (len<=0)
	{
		fprintf(stdout,"send reply failed,reason:%s\n",strerror(errno));
	}
	
}

void unimplement(int client_sock){
	const char *reply="\
<html>\n\
<head>\n\
<meta charset=\"UTF-8\">\n\
<title>Method Not Implemented</title>\n\
</head>\n\
<body>\n\
	<p>HTTP request method not supported!\n\
</body>\n\
</html>";
	int len=write(client_sock,reply,strlen(reply));
	if (len<=0)
	{
		fprintf(stdout,"send reply failed,reason:%s\n",strerror(errno));
	}

}

void not_found(int client_sock){
	const char *reply="\
<html>\n\
<head>\n\
<meta charset=\"UTF-8\">\n\
<title>NOT FOUND</title>\n\
</head>\n\
<body>\n\
	<p>request NOT FOUND!\n\
	<p>出错！\n\
</body>\n\
</html>";
	int len=write(client_sock,reply,strlen(reply));
	if (len<=0)
	{
		fprintf(stdout,"send reply failed,reason:%s\n",strerror(errno));
	}
}

int main()
{
	int sock;
	int i;
	struct sockaddr_in server_addr;

	//AF_INET 网络通信协议家族,SOCK_STREAM TCP协议
	sock = socket(AF_INET, SOCK_STREAM, 0);

	//清空标签，写地址和端口号
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET; //选择协议族ipv4
	//监听本地IP地址,htonl调整字节顺序
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT); //绑定端口号

	//实现绑定
	bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

	//128同一时间允许向服务端发送客户端的数量
	listen(sock, 128);

	//等待客户端的连接
	printf("等待客户端的连接\n");

	int done = 1;
	while (done)
	{
		struct sockaddr_in client;
		int client_sock, len;
		char client_ip[64];
		char buf[256];
		char method[16];
		char url[256];

		socklen_t client_addr_len;
		client_addr_len = sizeof(client);
		client_sock = accept(sock, (struct sockaddr *)&client, &client_addr_len);

		//打印客户端ip地址和端口号
		printf("client ip : %s\t port :%d\n",
			   inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, sizeof(client_ip)),
			   ntohs(client.sin_port));

		//处理http请求
		do_http_request(client_sock);

		len = write(client_sock, buf, len);

		printf("write finished,len:%d\n", len);

		close(client_sock);
	}

	return 0;
}
