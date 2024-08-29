#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h> // "hton" and "ntoh" (see below)
#include <sys/types.h> // types like "size_t"
#include <sys/stat.h> // stats about file like "fstat"
#include <unistd.h> // access posix API like "read"
#include <string.h> // manipulate array of chars

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
	int i = 0;
	for (; i < dbhdr->count; i++) {
		printf("Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
	printf("%s\n", addstring);

	// strtok gets tokens from a string and keeps track internally of the position
	// thus can be called with NULL to operate on the same string as before
	char *name = strtok(addstring, ",");
	char *addr = strtok(NULL, ",");
	char *hours = strtok(NULL, ",");

	// strncpy to safely copy our data into the in-memory struct
	strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
	strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));

	// convert ascii to int
	employees[dbhdr->count-1].hours = atoi(hours);

	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	int count = dbhdr->count;

	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == NULL) {
		printf("Calloc failed\n");
		return STATUS_ERROR;
	}

	// we already read the header so the cursor in the file is set to the employees
	read(fd, employees, count*sizeof(struct employee_t));

	int i = 0;
	for (; i < count; i++) {
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;
	return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) { 
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	int realCount = dbhdr->count;

	// host to network (hton) is needed because of endiness
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t)*realCount));
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

	// now that the file is open if we write to it we would append at the end
	// so we need to put the cursor to the start of the file
	lseek(fd, 0, SEEK_SET);

	write(fd, dbhdr, sizeof(struct dbheader_t));

	int i = 0;
	// we have to use the count value before htons
	for (; i < realCount; i++) {
		employees[i].hours = htonl(employees[i].hours);
		write(fd, &employees[i], sizeof(struct employee_t));
	}
	
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
	// we read from the file the length specified in our header struct
	// if it does not match the actual length of the file something went wrong
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

