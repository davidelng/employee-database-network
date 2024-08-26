#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {

}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {

}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {

}

int output_file(int fd, struct dbheader_t *dbhdr) { 
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	// host to network (hton) is needed because of endiness
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(dbhdr->filesize);
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

	// now that the file is open if we write to it we would append at the end
	// so we need to put the cursor to the start of the file
	lseek(fd, 0, SEEK_SET);

	write(fd, dbhdr, sizeof(struct dbheader_t));
	
	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		printf("Calloc failed to create db header\n");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
		perror("read");
		free(header);
		return STATUS_ERROR;
	}

	// network to host (ntoh) is needed because not all machine have the same endiness
	// 0x00000001 could be 0x010000000 on another machine
	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic); //LLAD
	header->filesize = ntohl(header->filesize);

	if (header->magic != HEADER_MAGIC) {
		printf("Improper header value\n");
		free(header);
		return -1;
	}

	if (header->version != 1) {
		printf("Improper header version\n");
		free(header);
		return -1;
	}

	struct stat dbstat = {0};
	fstat(fd, &dbstat);
	// we read from the file the length of our struct
	// if the length of our struct is not the length of the actual file something is wrong
	if (header->filesize != dbstat.st_size) {
		printf("Corrupted database\n");
		free(header);
		return -1;
	}

	*headerOut = header;

	return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
	// allocate a chunk of memory for the file header
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		printf("Calloc failed to create db header\n");
		return STATUS_ERROR;
	}

	// fill the struct with the first info in the file
	header->version = 0x1;
	header->count = 0;
	header->magic = HEADER_MAGIC;
	header->filesize = sizeof(struct dbheader_t);

	// set the struct pointer in main to point to the newly created struct
	// we need a double pointer for this
	*headerOut = header;

	return STATUS_SUCCESS;
}


