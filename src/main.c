#include <stdio.h> // standard I/O like "open" and "printf"
#include <stdlib.h> // standard library with "atoi", NULL, etc
#include <stdbool.h> // create a bool type that is basically 0 and 1
#include <getopt.h> // getopt library to create a cli
#include <unistd.h> // access posix API like "read"
#include <string.h> // manipulate array of chars

#include "common.h"
#include "file.h"
#include "parse.h"

// print a unix-like help message
void print_usage(char *argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n  create new database file\n");
	printf("\t -f  <filepath> (required) path to database file\n");
	printf("\t -l  list all employees\n");
	printf("\t -a  <name,address,hours> add a new employee\n");
	printf("\t -u  <id,hours> update hours for employee\n");
	printf("\t -d  <id> delete an employee\n");
	return;
}

int main(int argc, char* argv[]) { 

	int c = 0;
	bool newfile = false;
	char* filepath = NULL;
	bool list = false;
	char* addstring = NULL;
	char* updatestring = NULL;
	unsigned int deleteID = 0;

	int dbfd = -1; // -1 so we do not use it incorrectly as file descriptor
	dbheader* dbhdr = NULL; // we pass this between files of our program
	employee* employees = NULL; // same here

	// add a colon to the flag if it has data (optarg)
	while ((c = getopt(argc, argv, "nf:la:u:d:")) != -1) {
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
		employees = realloc(employees, dbhdr->count*(sizeof(employee)));
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

	if (output_file(filepath, dbhdr, employees) != STATUS_SUCCESS) {
		printf("Couldn't write to file\n");
		return -1;
	}

	return 0;
}
