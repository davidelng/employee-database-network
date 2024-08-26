#ifndef PARSE_H
#define PARSE_H

#define HEADER_MAGIC 0x4c4c4144 //LLAD

struct dbheader_t {
	unsigned int magic; // it's a "secret" value that we use to identify our file so we know that we can parse it
	unsigned short version;
	unsigned short count;
	unsigned int filesize;
};

struct employee_t {
	char name[256];
	char address[256];
	unsigned int hours;
};

int create_db_header(int fd, struct dbheader_t **headerOut);
int validate_db_header(int fd, struct dbheader_t **headerOut);
int read_employees(int fd, struct dbheader_t *, struct employee_t **employeesOut);
int output_file(int fd, struct dbheader_t *);
void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees);
int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring);

#endif
