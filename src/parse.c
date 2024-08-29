#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h> // "hton" and "ntoh" (see below)

#include "common.h"
#include "parse.h"

int create_db_header(dbheader** headerOut) {
	// allocate a chunk of memory for the file header
	dbheader* header = calloc(1, sizeof(dbheader));
	if (header == NULL) {
		printf("Calloc failed to create db header\n");
		return STATUS_ERROR;
	}

	// fill the struct with the first info in the file
	header->magic = HEADER_MAGIC;
	header->version = HEADER_VERSION;
	header->filesize = sizeof(dbheader);
	header->count = 0;
	header->lastID = 0;

	// set the struct pointer in main to point to the newly created struct
	// we need a double pointer for this
	*headerOut = header;

	return STATUS_SUCCESS;
}

int validate_db_header(int fd, dbheader** headerOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	dbheader* header = calloc(1, sizeof(dbheader));
	if (header == NULL) {
		printf("Calloc failed to create db header\n");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(dbheader)) != sizeof(dbheader)) {
		perror("read");
		free(header);
		return STATUS_ERROR;
	}

	// network to host (ntoh) is needed because not all machine have the same endiness
	// 0x00000001 could be 0x010000000 on another machine
	header->magic = ntohl(header->magic); //LLAD
	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->filesize = ntohl(header->filesize);
	header->lastID = ntohl(header->lastID);

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
	if (fstat(fd, &dbstat) == -1) {
        perror("fstat");
        free(header);
        return STATUS_ERROR;
    }

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

int output_file(char* filepath, dbheader* dbhdr, employee* employees) {
	char temp[256];
    if (snprintf(temp, sizeof(temp), "%s.temp", filepath) < 0) {
        perror("snprintf");
        return STATUS_ERROR;
    }
    int fd = open(temp, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		printf("Couldn't create temp file\n");
		perror("open");
		return STATUS_ERROR;
	}

	int realCount = dbhdr->count;

	// host to network (hton) is needed because of endiness
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(dbheader) + (sizeof(employee)*realCount));
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);
	dbhdr->lastID = htonl(dbhdr->lastID);

	// now that the file is open if we write to it we would append at the end
	// so we need to put the cursor to the start of the file
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        return STATUS_ERROR;
    }

    if (write(fd, dbhdr, sizeof(dbheader)) != sizeof(dbheader)) {
        perror("write");
        return STATUS_ERROR;
    }

	// we have to use the count value before htons
	for (int i = 0; i < realCount; i++) {
		employees[i].hours = htonl(employees[i].hours);
        if (write(fd, &employees[i], sizeof(employee)) != sizeof(employee)) {
            perror("write");
            return STATUS_ERROR;
        }
	}

	if (rename(temp, filepath) < 0) {
        perror("rename");
        return STATUS_ERROR;
    }
	
	return STATUS_SUCCESS;
}

int read_employees(int fd, dbheader* dbhdr, employee** employeesOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	employee* employees = calloc(dbhdr->count, sizeof(employee));
	if (employees == NULL) {
		printf("Calloc failed\n");
		return STATUS_ERROR;
	}

    if (dbhdr->count > 0) {
		// we already read the header so the cursor in the file is set to the employees
		if (read(fd, employees, dbhdr->count*sizeof(employee)) != dbhdr->count*sizeof(employee)) {
            perror("read");
            free(employees);
            return STATUS_ERROR;
		}

		for (int i = 0; i < dbhdr->count; i++) {
			employees[i].hours = ntohl(employees[i].hours);
		}
	}

	*employeesOut = employees;
	return STATUS_SUCCESS;
}

int add_employee(dbheader* dbhdr, employee* employees, char* addstring) {
	printf("Adding %s\n", addstring);

	// strtok gets tokens from a string and keeps track internally of the position
	// thus can be called with NULL to operate on the same string as before
	char *name = strtok(addstring, ",");
	char *addr = strtok(NULL, ",");
	char *hours = strtok(NULL, ",");

	if (name == NULL || addr == NULL || hours == NULL) {
        printf("Missing params\n");
        return STATUS_ERROR;
    }

	employees[dbhdr->count-1].ID = dbhdr->lastID;
	// convert ascii to int
	employees[dbhdr->count-1].hours = atoi(hours);
	// strncpy to safely copy our data into the in-memory struct
	strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
	strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));

	return STATUS_SUCCESS;
}

int update_employee(dbheader* dbhdr, employee* employees, char* updatestring) {
	char* stringID = strtok(updatestring, ",");
	char* hours = strtok(NULL, ",");

	if (stringID == NULL || hours == NULL) {
		printf("Missing params\n");
		return STATUS_ERROR;
	}

	unsigned int updateID = atoi(stringID);
	for (int i = 0; i < dbhdr->count; i++) {
		if (employees[i].ID == updateID) {
			employees[i].hours = atoi(hours);
			printf("Employee %d updated\n", updateID);
			break;
		}
	}

	return STATUS_SUCCESS;
}

int delete_employee(dbheader* dbhdr, employee** employeesOut, unsigned int deleteID) {
    bool found = false;
    for (int i = 0; i < dbhdr->count; i++) {
        if ((*employeesOut)[i].ID == deleteID) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("Employee %d does not exist\n", deleteID);
        return STATUS_SUCCESS;
    }

    dbhdr->count--;
	employee* newEmployees = calloc(dbhdr->count, sizeof(employee));
    if (newEmployees == NULL) {
        printf("Calloc failed\n");
        return STATUS_ERROR;
    }

    int i = 0, j= 0;
    for (; i < dbhdr->count+1; i++) {
        if ((*employeesOut)[i].ID == deleteID) {
            printf("Deleting %d\n", deleteID);
            continue;
        }
        newEmployees[j].ID = (*employeesOut)[i].ID;
        newEmployees[j].hours = (*employeesOut)[i].hours;
        strncpy(newEmployees[j].name, (*employeesOut)[i].name, sizeof(newEmployees[j].name));
        strncpy(newEmployees[j].address, (*employeesOut)[i].address, sizeof(newEmployees[j].address));
        j++;
    }

	free(*employeesOut);
    *employeesOut = newEmployees;
	
    printf("Employee %d deleted\n", deleteID);

    return STATUS_SUCCESS;
}

void list_employees(dbheader* dbhdr, employee* employees) {
	for (int i = 0; i < dbhdr->count; i++) {
		printf("Employee %d\n", employees[i].ID);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
}
