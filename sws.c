/*
 * Rogan McNeill
 * V00791096
 * CSC361 Assignment p1
 * Simple UDP Webserver
 * Winter 2017
 */

#include "sws.h"

int main(int argc, char *argv[])
{
  int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in sa;
  char buffer[ BUFFER_SIZE ];
  char* dir;
  char* port;
  char ip[16];
  int port_num;
  ssize_t recsize;
  socklen_t fromlen;
  struct in_request* request;

  // check for correct arguments
  if(argc != 3) {
    printf("To run the Simple Web Server use:\n./sws <port> <path to file(s)>\n");
    exit(1);
  }
 
  // work arguments into variables
  port = malloc(strlen(argv[1]) + 1);
  strncpy(port, argv[1], strlen(argv[1]) + 1); 
  port_num = (atoi(port));
  
  dir = malloc(strlen(argv[2]) + 1);
  strncpy(dir, argv[2], strlen(argv[2]) + 1);

  // check that the provided directory exists
  struct stat sb;

  if (!(stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode))) {
    printf("Directory \"%s\" does not exist. Exiting.\n", dir);
    free(dir);
    free(port);
    exit(1);
  }

  // setup the socket address and port
  memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port_num);
  fromlen = sizeof(sa);

  // prepare the buffer
  memset(buffer, 0, sizeof buffer*sizeof(char));

  // bind the socket
  if (-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)) {
    perror("error bind failed");
    close(sock);
    exit(EXIT_FAILURE);
  }

  // notify the console that the server is running
  printf("sws is running on UDP port %d and serving %s\n", port_num, dir);
  printf("press \'q\' and enter to quit ...\n");

  // set the socket to reuse the address
  int opt = TRUE;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
  // file descriptor setup
  fd_set f_descs;
  int max_desc = 0;
  int result;
  struct timeval timeout;

  // determine max FD
  if(sock > STDIN_FILENO) {
    max_desc = sock;
  } else {
    max_desc = STDIN_FILENO;
  }

  // infinite loop
  for (;;) {
    
    // zero the descriptors
    FD_ZERO(&f_descs);
    
    // set em up
    FD_SET(STDIN_FILENO, &f_descs);
    FD_SET(sock, &f_descs);
    
    // 2 second timeout seems appropriate
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    // select the file descriptor with something in its buffer
    result = select(max_desc + 1, &f_descs, NULL, NULL, &timeout);

    if (FD_ISSET(sock, &f_descs)) {

      // if it's the UDP socket let's handle it
      recsize = recvfrom(sock, (void*)buffer, sizeof buffer, 0, (struct sockaddr*)&sa, &fromlen);

      if (recsize < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }

      // parse the request
      request = parse_request(buffer, dir);

      // clear the buffer
      memset(buffer, 0, sizeof buffer*sizeof(char));
      
      // process response
      respond(request, sock, sa);  
               
      // get the IP for logging
      inet_ntop(AF_INET, &sa.sin_addr, ip, sizeof(ip));
      
      // log the result
      log(request, ip, port);

    } else if(FD_ISSET(STDIN_FILENO, &f_descs)) {
      
      // if instead it was the sandard input, lets handle that
      
      // get what was input through stdin
      fgets(buffer, 100, stdin);
      
      // check if it was a q, and if it was, terminate the server
      if (!strncmp(buffer, "q\n", 2)){
        printf("Server terminated by user...\n");
        close(sock);
        exit(EXIT_SUCCESS);
      }
    }
  }
}

/*
 * respond()
 * Takes as input the struct, and socket data
 * then determines the appropraite response and
 * delivers it along with the file if necessary.
 */
void respond(struct in_request* request, int sock, struct sockaddr_in sa){
  int bytes_sent;
  char* response;
  char buffer[ BUFFER_SIZE ];
  
  // open the file  
  FILE *fp;
  fp = fopen(request->file_path, "r");

  // check the request components to determine the nature of the request
  if((strcmp(request->type, "GET") != 0 && strcmp(request->type, "get") != 0 ) || (strcmp(request->protocol, "HTTP/1.0\r\n\r\n") != 0 && strcmp(request->protocol, "http/1.0\r\n\r\n"))) {
    request->status = 400;
  } else if (fp == NULL || strcmp(request->file, "/../") == 0){
    request->status = 404;
  } else {
    request->status = 200;
  }
  
  // based on the above result, lets build the response string
  switch(request->status) {
    case 200:
      response = "HTTP/1.0 200 OK\r\n\r\n";
      break;
    case 400:
      response = "HTTP/1.0 400 Bad Request\r\n\r\n";
      break;
    case 404:
      response = "HTTP/1.0 404 Not Found\r\n\r\n";
      break;
    case 501:
      response = "HTTP/1.0 501 Not Implemented\r\n\r\n";
      break;
  }

  // now send the response header
  bytes_sent = sendto(sock, (void*)response, strlen(response), 0, (struct sockaddr*)&sa, sizeof sa);

  if (bytes_sent < 0) {
    printf("Error sending packet: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // only if the request status was 200 should we do anything with the file
  if(request->status == 200) {

    // lets send one segment at a time until fgets returns null
    while (fgets(buffer, sizeof(buffer), fp)) {

      bytes_sent = sendto(sock, (void*)buffer, strlen(buffer), 0, (struct sockaddr*)&sa, sizeof sa);

      if (bytes_sent < 0) {
        printf("Error sending packet: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
    
    fclose(fp);
  }
}

/*
 * log()
 * Takes as input the struct, ip and port
 * and produces the console log output
 */
void log(const struct in_request* request, const char* ip, const char* port){
  
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  
  char* mon;
  char* res;
  char* response;
  char* protocol_copy;
  char* protocol;

  // allocate memory for temporary variables, copy the protocol so that manipulation does not affect original
  protocol_copy = malloc(strlen(request->protocol) + 1);
  protocol = malloc(strlen(request->protocol) + 1);
  protocol[0] = '\0';
  strncpy(protocol_copy, request->protocol, strlen(request->protocol) + 1);
  protocol = strtok(protocol_copy, "\r\n\r\n");
  
  // convert the month to MMM format needed
  switch (tm.tm_mon + 1) {
    case 1:
      mon = "Jan";
      break;
    case 2:
      mon = "Feb";
      break;
    case 3:
      mon = "Mar";
      break;
    case 4:
      mon = "Apr";
      break;
    case 5:
      mon = "May";
      break;
    case 6:
      mon = "Jun";
      break;
    case 7:
      mon = "Jul";
      break;
    case 8:
      mon = "Aug";
      break;
    case 9:
      mon = "Sep";
      break;
    case 10:
      mon = "Oct";
      break;
    case 11:
      mon = "Nov";
      break;
    case 12:
      mon = "Dec";
      break;
  }

  // generate a response line for the log
  switch(request->status) {
    case 200:
      res = "HTTP/1.0 200 OK";
      break;
    case 400:
      res = "HTTP/1.0 400 Bad Request";
      break;
    case 404:
      res = "HTTP/1.0 404 Not Found";
      break;
    default:
      res = "HTTP/1.0 501 Not Implemented";
      break;
  }

  // log to console
  printf("%s %02d %02d:%02d:%02d %s:%s %s %s %s; %s; %s\n", mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ip, port, request->type, request->file, protocol, res, request->file_path);
}

/*
 * parse_request()
 * Takes the buffered request and directory being served
 * and builds up the struct with the parsed data.
 * Returns a struct with all the relevant data for the response
 */
struct in_request* parse_request(const char* origin_request, const char* dir){
  char* request_copy;
  char* type;
  char* file;
  char* protocol;
  char* full_path;
  
  struct in_request* request;
  
  // allocate the memory for the struct
  request = malloc(sizeof(struct in_request));
  
  // allocate and copy the original request for break down
  request_copy = malloc(strlen(origin_request) + 1);
  strncpy(request_copy, origin_request, strlen(origin_request) + 1);
  
  // allocate and copy in the request type
  type = strtok(request_copy, " ");
  request->type = malloc(strlen(type) + 1);
  strncpy(request->type, type, strlen(type) + 1);
  
  // allocate and cope in the file requested
  file = strtok(NULL, " ");
  request->file = malloc(strlen(file) + 1);
  strncpy(request->file, file, strlen(file) + 1); 
  
  // allocate and copy in the protocol version
  protocol = strtok(NULL, " ");
  request->protocol = malloc(strlen(protocol) + 1);
  strncpy(request->protocol, protocol, strlen(protocol) + 1);
  
  // allocate and construct the full path to the file
  if(strcmp(file, "/") == 0) {
    full_path = malloc(strlen(dir) + strlen("/index.html") + 1);
    full_path[0] = '\0';
    strcat(full_path, dir);
    strcat(full_path, "/index.html");
  } else {
    full_path = malloc(strlen(file) + strlen(dir) + 1);
    full_path[0] = '\0';
    strcat(full_path, dir);
    strcat(full_path, file);
  }
  
  request->file_path = malloc(strlen(full_path) + 1);
  strncpy(request->file_path, full_path, strlen(full_path) + 1);
  
  // return the parsed request
  return request;  
}
