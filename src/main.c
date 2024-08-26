#include <stdio.h> // standard I/O like "open" and "printf"
#include <stdbool.h> // create a bool type that is basically 0 and 1
#include <getopt.h> // getopt library to create a cli
#include <stdlib.h> // standard library with "atoi", NULL, etc

#include "common.h"
#include "file.h"
#include "parse.h"

// print a unix-like help message
void print_usage(char *argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n  - create new database file\n");
	printf("\t -f  - (required) path to database file\n");
	return;
}

int main(int argc, char *argv[]) { 

	int c;
	bool newfile = false;
	char *filepath = NULL;
	char *addstring = NULL;

	int dbfd = -1; // -1 so we do not use it incorrectly as file descriptor
	struct dbheader_t *dbhdr = NULL; // we pass this between files of our program
	struct employee_t *employees = NULL; // same here

	// add a colon to the flag if it has data (optarg)
	while ((c = getopt(argc, argv, "nf:a:")) != -1) {
		switch(c) {
			case 'n': // single quotes because they are char and not a string literals (array of chars)
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
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

		if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			printf("Failed to create database header\n");
			return -1;
		}
	} else {
		dbfd = open_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to open database file\n");
			return -1;
		}

		if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			printf("Failed to validate database header\n");
			return -1;
		}
	}

	if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
		printf("Failed to read employees\n");
		return 0;
	}

	if (addstring) {
		// if we add an employee we have to allocate new memory for it
		dbhdr->count++;
		employees = realloc(employees, dbhdr->count*(sizeof(struct employee_t)));
		add_employee(dbhdr, employees, addstring);
	}

	output_file(dbfd, dbhdr, employees);

	return 0;
}
