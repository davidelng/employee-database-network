#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144 //LLAD
#define HEADER_VERSION 0x1

typedef struct {
	unsigned short version;
	unsigned short count;
	unsigned int lastID;
	unsigned int magic; // it's a "secret" value that we use to identify our file so we know that we can parse it
	unsigned int filesize;
} dbheader_t;

typedef struct {
	char name[256];
	char address[256];
	unsigned int ID;
	unsigned int hours;
} employee_t;

int create_db_header(dbheader_t** headerOut);
int validate_db_header(int fd, dbheader_t** headerOut);
int read_employees(int fd, dbheader_t* dbhdr, employee_t** employeesOut);
int output_file(int fd, dbheader_t* dbhdr, employee_t* employees);
int add_employee(dbheader_t* dbhdr, employee_t** employees, char* addstring);
int update_employee(dbheader_t* dbhdr, employee_t* employees, char* updatestring);
int delete_employee(dbheader_t* dbhdr, employee_t** employeesOut, unsigned int deleteID);
void list_employees(dbheader_t* dbhdr, employee_t* employees);

#endif
