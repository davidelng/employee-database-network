#include <stdio.h> // standard I/O like "open" and "printf"
#include <stdlib.h> // standard library with "atoi", NULL, etc
#include <stdbool.h> // create a bool type that is basically 0 and 1
#include <getopt.h> // getopt library to create a cli
#include <unistd.h> // access posix API like "read"
#include <string.h> // manipulate array of chars
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"
#include "srvpoll.h"

clientstate_t clientStates[MAX_CLIENTS] = {0};

// print a unix-like help message
void print_usage(char* argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n  create new database file\n");
	printf("\t -f  <filepath> (required) path to database file\n");
	printf("\t -l  list all employees\n");
	printf("\t -a  <name,address,hours> add a new employee\n");
	printf("\t -u  <id,hours> update hours for employee\n");
	printf("\t -d  <id> delete an employee\n");
	return;
}

void poll_loop(unsigned short port, dbheader_t* dbhdr, employee_t* employees) {
	// listen_fd -> fd creato per accettare la chiamata
	// conn_fd -> quando accettiamo, salviamo qui temporaneamente il fd
	// freeSlot -> prossimo slot disponibile
	int listen_fd, conn_fd, freeSlot;

	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);

	memset(&server_addr, 0, sizeof(server_addr)); // settiamo server_addr a nulla
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; // every ip addr owned by our machine
	server_addr.sin_port = htons(port); // network endian (big endian)

	// struct that has a fd to track, an event to track and the received event
	struct pollfd fds[MAX_CLIENTS+1];
	int nfds = 1;
	int opt = 1;

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// make the socket non waiting
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// 10 is backlog, number of pending connection to have in queue
	if (listen(listen_fd, 10) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	init_clients(clientStates);
	memset(fds, 0, sizeof(fds));
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN;

	printf("Server listening on port %d\n", port);
	
	while(1) {
		// add active connections to read set
		int j = 1; // offset di uno per skippare il listen_fd
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (clientStates[i].fd != -1) {
				fds[j].fd = clientStates[i].fd;
				fds[j].events = POLLIN;
				j++;
			}
		}

		// wait for an event on one of the sockets
		// pass poll fd struct, n of fd we're tracking and timeout (-1 no timeout)
		int n_events = poll(fds, nfds, -1);
		if (n_events == -1) {
			perror("poll");
			exit(EXIT_FAILURE);
		}

		// check for new  connections
		if (fds[0].revents & POLLIN) {
			if ((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) {
				perror("accept");
				continue;
			}

			printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		
			freeSlot = find_free_slot(clientStates);
			if (freeSlot == -1) {
				printf("Server full: closing new connections\n");
				close(conn_fd);
			} else {
				clientStates[freeSlot].fd = conn_fd;
				clientStates[freeSlot].state = STATE_CONNECTED;
				nfds++;
				printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
			}

			n_events--;
		}

		// check each client for read/write activity
		for (int i = 1; i < nfds && n_events > 0; i++) {
			if (fds[i].revents & POLLIN) {
				n_events--;

				int fd = fds[i].fd;
				int slot = find_slot_by_fd(clientStates, fd);
				ssize_t bytes_read = read(fd, &clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
				if (bytes_read <= 0) {
					close(fd);
					if (slot == -1) {
						printf("Tried to close fd that doesn't exist\n");
					} else {
						clientStates[slot].fd = -1;
						clientStates[slot].state = STATE_DISCONNECTED;
						printf("Client disconnected or error\n");
						nfds--;
					}
				} else {
					handle_client_fsm(dbhdr, employees, &clientStates[slot]);
				}
			}
		}
	}
}

int main(int argc, char* argv[]) { 

	int c = 0;
	int port = 8080;
	bool newfile = false;
	char* filepath = NULL;
	bool list = false;
	char* addstring = NULL;
	char* updatestring = NULL;
	unsigned int deleteID = 0;

	int dbfd = -1; // -1 so we do not use it incorrectly as file descriptor
	dbheader_t* dbhdr = NULL; // we pass this between files of our program
	employee_t* employees = NULL; // same here

	// add a colon to the flag if it has data (optarg)
	while ((c = getopt(argc, argv, "nf:la:u:d:p:")) != -1) {
		switch(c) {
			case 'n': // single quotes because they are char and not a string literals (array of chars)
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'u':
				updatestring = optarg;
				break;
			case 'd':
				deleteID = atoi(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case '?':
				printf("Unknown option -%c\n", c);
				break;
			default:
				return -1;
		}
	}

	if (filepath == NULL) {
		printf("Filepath is a required argument\n");
		print_usage(argv);
		return 0;
	}

	if (newfile) {
		dbfd = create_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to create database file\n");
			return -1;
		}

		if (create_db_header(&dbhdr) != STATUS_SUCCESS) {
			close(dbfd);
			printf("Failed to create database header\n");
			return -1;
		}
	} else {
		dbfd = open_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to open database file\n");
			return -1;
		}

		if (validate_db_header(dbfd, &dbhdr) != STATUS_SUCCESS) {
			close(dbfd);
			printf("Failed to validate database header\n");
			return -1;
		}
	}

	if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
		close(dbfd);
		printf("Failed to read employees\n");
		return 0;
	}

	if (addstring != NULL) {
		// if we add an employee we have to allocate new memory for it
		dbhdr->lastID++;
		dbhdr->count++;
		employees = realloc(employees, dbhdr->count*(sizeof(employee_t)));
		if (employees == NULL) {
			printf("Realloc failed\n");
			return -1;
		}
		if (add_employee(dbhdr, employees, addstring) != STATUS_SUCCESS) {
			printf("Couldn't add employee\n");
			return -1;
		}
	}

	if (updatestring != NULL && update_employee(dbhdr, employees, updatestring) != STATUS_SUCCESS) {
            printf("Couldn't update employee\n");
            return -1;
        }

	if (deleteID > 0 && delete_employee(dbhdr, &employees, deleteID) != STATUS_SUCCESS) {
            printf("Couldn't delete employee\n");
            return -1;
        }

	if (list) {
		list_employees(dbhdr, employees);
	}

	poll_loop(port, dbhdr, employees);

	if (output_file(filepath, dbhdr, employees) != STATUS_SUCCESS) {
		printf("Couldn't write to file\n");
		return -1;
	}

	if (dbhdr != NULL) {
		free(dbhdr);
	}
	if (employees != NULL) {
		free(employees);
	}

	return 0;
}
