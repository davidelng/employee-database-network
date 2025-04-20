#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "parse.h"

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

int send_employee(int fd, char* addstr) {
	char buff[4096] = {0};

	dbproto_hdr_t* hdr = buff;
	hdr->type = MSG_EMPLOYEE_ADD_REQ;
	hdr->len = 1;

	dbproto_employee_add_req* employee = (dbproto_employee_add_req*)&hdr[1];
	// avoid buffer overflow, the length of the copy is the length of the destination
	strncpy(&employee->data, addstr, sizeof(employee->data));

	hdr->type = htonl(hdr->type);
	hdr->len = htons(hdr->len);

	// write the hello message
	write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_req));

	// recv the response
	read(fd, buff, sizeof(buff));

	hdr->type = ntohl(hdr->type);
	hdr->len = ntohs(hdr->len);

	if (hdr->type == MSG_ERROR) {
		printf("Improper format for add employee string\n");
		close(fd);
		return STATUS_ERROR;
	}

	if (hdr->type == MSG_EMPLOYEE_ADD_RESP) {
		printf("Employee succesfully added\n");
	}

	return STATUS_SUCCESS;
}

int list_employees_in_db(int fd) {
	char buff[4096] = {0};

	dbproto_hdr_t* hdr = buff;
	hdr->type = MSG_EMPLOYEE_LIST_REQ;
	hdr->len = 1;

	dbproto_employee_list_req* employee = (dbproto_employee_list_req*)&hdr[1];

	hdr->type = htonl(hdr->type);
	hdr->len = htons(hdr->len);

	// write the hello message
	write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_list_req));

	// recv the response
	read(fd, buff, sizeof(buff));

	hdr->type = ntohl(hdr->type);
	hdr->len = ntohs(hdr->len);

	if (hdr->type == MSG_ERROR) {
		printf("Couldn't list employees\n");
		close(fd);
		return STATUS_ERROR;
	}

	if (hdr->type == MSG_EMPLOYEE_LIST_RESP) {
		printf("Listing employees:\n");
		dbproto_employee_list_resp* employee = (dbproto_employee_list_resp*)&hdr[1];

		for (int i = 0; i < hdr->len; i++) {
			read(fd, employee, sizeof(dbproto_employee_list_resp));
			employee->hours = ntohl(employee->hours);
			printf("%s, %s, %d\n", employee->name, employee->address, employee->hours);
		}
	}

	return STATUS_SUCCESS;
}

int main(int argc, char* argv[]) {
	char* addarg = NULL;
	char* portarg = NULL;
	char* hostarg = NULL;
	unsigned short port = 0;
	bool list = false;

	int c;
	while ((c = getopt(argc, argv, "h:p:a:l")) != -1) {
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
			case 'l':
				list = true;
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

	if (addarg != NULL) {
		if (send_employee(fd, addarg) != STATUS_SUCCESS) {
			return -1;
		}
	}

	if (list) {
		list_employees_in_db(fd);
	}

	close(fd);

	return 0;
}
