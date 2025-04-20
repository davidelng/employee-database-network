#ifndef SRVPOLL_H
#define SRVPOLL_H

#include "parse.h"
#include <poll.h>
#include "common.h"

#define MAX_CLIENTS 256
#define BUFF_SIZE 4096

typedef enum {
	STATE_NEW,
	STATE_CONNECTED,
	STATE_DISCONNECTED,
	STATE_HELLO,
	STATE_MSG,
	STATE_GOODBYE,
} state_e;

typedef struct {
	int fd;
	state_e state;
	char buffer[BUFF_SIZE];
} clientstate_t;

void init_clients(clientstate_t* states);

int find_free_slot(clientstate_t* states);

int find_slot_by_fd(clientstate_t* states, int fd);

void handle_client_fsm(dbheader_t* dbhdr, int dbfd, employee_t** employees, clientstate_t* client);

void fsm_reply_hello(clientstate_t* client, dbproto_hdr_t* hdr);

void fsm_reply_hello(clientstate_t* client, dbproto_hdr_t* hdr);

void fsm_reply_add(clientstate_t* client, dbproto_hdr_t* hdr);

void fsm_reply_add_err(clientstate_t* client, dbproto_hdr_t* hdr);

#endif
