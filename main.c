#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
int main() {
    int sockfd;
    // * AF_INT: IPv4
    // * 以数据流的方式传输——TCP协议
    // * SOCK_DGRAM：数据报文的方式传输——UDP协议
    // * 0: 默认协议（会选取最为合适的协议）
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    // * INADDR_ANY: 本机的所有IP地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // * 端口号
    serv_addr.sin_port = htons(BASIC_SERVER_PORT);
    bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    // * 5: 最大连接数
    listen(sockfd, 5);
}