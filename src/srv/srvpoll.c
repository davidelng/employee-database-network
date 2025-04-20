#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"
#include "srvpoll.h"

void fsm_reply_hello(clientstate_t* client, dbproto_hdr_t* hdr) {
	hdr->type = htonl(MSG_HELLO_RESP);
	hdr->len = htons(1);
	dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
	hello->proto = htons(PROTO_VER);

	write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp));
}

void fsm_reply_hello_err(clientstate_t* client, dbproto_hdr_t* hdr) {
	hdr->type = htonl(MSG_ERROR);
	hdr->len = htons(0);

	write(client->fd, hdr, sizeof(dbproto_hdr_t));
}

void fsm_reply_add(clientstate_t* client, dbproto_hdr_t* hdr) {
	hdr->type = htonl(MSG_EMPLOYEE_ADD_RESP);
	hdr->len = htons(1);
	dbproto_employee_add_resp* employee = (dbproto_employee_add_resp*)&hdr[1];

	write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_resp));
}

void fsm_reply_add_err(clientstate_t* client, dbproto_hdr_t* hdr) {
	hdr->type = htonl(MSG_ERROR);
	hdr->len = htons(0);

	write(client->fd, hdr, sizeof(dbproto_hdr_t));
}

void fsm_send_employee(clientstate_t* client, dbheader_t* dbhdr, employee_t** employees) {
	dbproto_hdr_t* hdr = (dbproto_hdr_t*)client->buffer;
	hdr->type = htonl(MSG_EMPLOYEE_LIST_RESP);
	hdr->len = htons(dbhdr->count);

	write(client->fd, hdr, sizeof(dbproto_hdr_t));

	dbproto_employee_list_resp *employee = (dbproto_employee_list_resp*)&hdr[1];

	employee_t* dbemployees = *employees;

	for (int i = 0; i < dbhdr->count; i++) {
		strncpy(&employee->name, dbemployees[i].name, sizeof(employee->name));
		strncpy(&employee->address, dbemployees[i].address, sizeof(employee->address));
		employee->hours = htonl(dbemployees[i].hours);
		write(client->fd, employee, sizeof(dbproto_employee_list_resp));
	}
}

void handle_client_fsm(dbheader_t* dbhdr, int dbfd, employee_t** employees, clientstate_t* client) {
	dbproto_hdr_t* hdr = (dbproto_hdr_t*)client->buffer;

	hdr->type = ntohl(hdr->type);
	hdr->len = ntohs(hdr->len);

	if (client->state == STATE_HELLO) {
		if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
			printf("Didn't get MSG_HELLO in HELLO state\n");
			// TODO send err message
		}

		dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
		hello->proto = ntohs(hello->proto);
		if (hello->proto != PROTO_VER) {
			printf("Protocol mismatch\n");
			fsm_reply_hello_err(client, hdr);
			return;
		}

		fsm_reply_hello(client, hdr);
		client->state = STATE_MSG;
		printf("Client upgraded to STATE_MSG\n");
	}

	if (client->state == STATE_MSG) {
		if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
			dbproto_employee_add_req* employee = (dbproto_employee_add_req*)&hdr[1];

			printf("Adding employee: %s\n", employee->data);
			if (add_employee(dbhdr, employees, employee->data) != STATUS_SUCCESS) {
				fsm_reply_add_err(client, hdr);
				return;
			} else {
				fsm_reply_add(client, hdr);
				output_file(dbfd, dbhdr, *employees);
			}
		}

		if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
			fsm_send_employee(client, dbhdr, employees);
		}
	}
}

void init_clients(clientstate_t* states) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		states[i].fd = -1;
		states[i].state = STATE_NEW;
		memset(&states[i].buffer, '\0', BUFF_SIZE);
	}
}

int find_free_slot(clientstate_t* states) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (states[i].fd == -1) {
			return i;
		}
	}
	return -1;
}

int find_slot_by_fd(clientstate_t* states, int fd) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (states[i].fd == fd) {
			return i;
		}
	}
	return -1;
}
