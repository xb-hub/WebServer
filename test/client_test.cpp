//
// Created by 许斌 on 2021/10/27.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int clientfd;
    struct sockaddr_in addr;
    char buf[1024];
    char response[1024];
    u_short port = (u_short)4000;
    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket creat fail!");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(clientfd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        printf("connect fail!");
        return -1;
    }
    sprintf(buf, "GET / HTTP/1.1\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Host: 127.0.0.1:4000\r\n");
    send(clientfd, buf, strlen(buf), 0);
    recv(clientfd, response, 1024, 0);
    printf("%s\n", response);
}