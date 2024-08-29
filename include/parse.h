#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144 //LLAD
#define HEADER_VERSION 0x1

typedef struct dbheader_t {
	unsigned short version;
	unsigned short count;
	unsigned int lastID;
	unsigned int magic; // it's a "secret" value that we use to identify our file so we know that we can parse it
	unsigned int filesize;
} dbheader;

typedef struct employee_t {
	char name[256];
	char address[256];
	unsigned int ID;
	unsigned int hours;
} employee;

int create_db_header(dbheader** headerOut);
int validate_db_header(int fd, dbheader** headerOut);
int read_employees(int fd, dbheader* dbhdr, employee** employeesOut);
int output_file(char* filepath, dbheader* dbhdr, employee* employees);
int add_employee(dbheader* dbhdr, employee* employees, char* addstring);
int update_employee(dbheader* dbhdr, employee* employees, char* updatestring);
int delete_employee(dbheader* dbhdr, employee** employeesOut, unsigned int deleteID);
void list_employees(dbheader* dbhdr, employee* employees);

#endif
