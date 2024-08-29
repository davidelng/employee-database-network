#include <stdio.h>
#include <fcntl.h> // operate with file descriptors
#include <sys/types.h> // types like "size_t"
#include <sys/stat.h> // stats about file like "fstat"

#include "common.h"

int create_db_file(char* filepath) {
	int fd = open(filepath, O_RDWR);
	if (fd != -1) {
		printf("File already exists\n");
		return STATUS_ERROR;
	}

	fd = open(filepath, O_RDWR | O_CREAT, 0644);
	if (fd == -1) {
		perror("open");
		return STATUS_ERROR;
	}

	return fd;
}

int open_db_file(char* filepath) {
	int fd = open(filepath, O_RDWR);
	if (fd == -1) {
		perror("open");
		return STATUS_ERROR;
	}

	return fd;
}
