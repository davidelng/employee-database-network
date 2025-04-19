#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "srvpoll.h"

void handle_client_fsm(dbheader_t* dbhdr, employee_t* employees, clientstate_t* client) {
	dbproto_hdr_t* hdr = (dbproto_hdr_t*)client->buffer;

	if (client->state == STATE_HELLO) {

	}

	if (client->state == STATE_MSG) {

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
