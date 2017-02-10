/*
 * Rogan McNeill
 * V00791096
 * CSC361 Assignment p1
 * Simple UDP Webserver
 * Winter 2017
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

// buffer size
#define BUFFER_SIZE 4096

#define TRUE 1
#define FALSE 0

// main data struct
struct in_request {
  int   status;
  char* type;
  char* file;
  char* file_path;
  char* protocol;
};

//function prototypes
struct in_request* parse_request(const char*, const char*);
void respond(struct in_request*, int, struct sockaddr_in);
void log(const struct in_request*, const char*, const char*);