#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

int send_hello(int fd) {
	char buff[4096] = {0};

	dbproto_hdr_t* hdr = buff;
	hdr->type = MSG_HELLO_REQ;
	hdr->len = 1;

	// send the hello request with the version we speak
	dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
	hello->proto = PROTO_VER;

	hdr->type = htonl(hdr->type);
	hdr->len = htons(hdr->len);
	hello->proto = htons(hello->proto);

	// write the hello message
	write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

	// recv the response
	read(fd, buff, sizeof(buff));

	hdr->type = ntohl(hdr->type);
	hdr->len = ntohs(hdr->len);

	if (hdr->type == MSG_ERROR) {
		printf("Protocol mismatch\n");
		close(fd);
		return STATUS_ERROR;
	}

	printf("Server connected, protocol v1\n");
	return STATUS_SUCCESS;
}

int main(int argc, char* argv[]) {
	char* addarg = NULL;
	char* portarg = NULL;
	char* hostarg = NULL;
	unsigned short port = 0;

	int c;
	while ((c = getopt(argc, argv, "h:p:a:")) != -1) {
		switch(c) {
			case 'a':
				addarg = optarg;
				break;
			case 'h':
				hostarg = optarg;
				break;
			case 'p':
				portarg = optarg;
				port = atoi(portarg);
				break;
			case '?':
				printf("Unknown option -%c\n", c);
				break;
			default:
				return -1;
		}
	}

	if (port == 0) {
		printf("Bad port %s\n", portarg);
		return -1;
	}

	if (hostarg == NULL) {
		printf("Must specify host with -h\n");
		return -1;
	}

	struct sockaddr_in serverInfo = {0};
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = inet_addr(hostarg);
	serverInfo.sin_port = htons(port);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("socket");
		return -1;
	}

	if (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
		perror("connect");
		close(fd);
		return 0;
	}

	if (send_hello(fd) != STATUS_SUCCESS) {
		return -1;
	}

	close(fd);

	return 0;
}
