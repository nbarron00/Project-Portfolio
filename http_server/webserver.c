#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERV_PORT 9007 /* port */
#define BACKLOG 10 /* max number of client connections */
#define MAXLINE 4096 /* max text line length */

int setup_TCP_connection() /* return original socket file descriptor */
{
  int sockfd;
  struct sockaddr_in servaddr;
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("error: unable to create socket: ");
      exit(1);
    }

  /* set address info */
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(servaddr.sin_zero, '\0', sizeof(servaddr.sin_zero));
  
  /* bind socket */
  if(bind(sockfd, (struct sockaddr *) &servaddr,
	  sizeof(servaddr)) < 0)
    {
      perror("error: unable to bind to socket: ");
      exit(1);
    }

  if (listen(sockfd, BACKLOG) < 0)
    {
      perror("error: unable to listen for incoming connections: ");
      exit(1);
    }
  printf("Server running... waiting for connections\n");

  return sockfd;
}

/*
return filetype
note: only takes valid filename less than 21 characters long
 */
char parse_HTTP_request(char* http_request, char* filename)
{
  int i = 5;
  int j = 0;
  char file_type = 'n';
  
  while(http_request[i] != ' ')
    {
      filename[j] = http_request[i];
      i++;
      j++;
    }
  filename[j] = '\0'; //end of string

  char jpg[5] = {'.', 'j', 'p', 'g', '\0'};
  char txt[5] = {'.', 't', 'x', 't', '\0'};
  char png[5] = {'.', 'p', 'n', 'g', '\0'};
  char html[6] = {'.', 'h', 't', 'm', 'l', '\0'};
  
  if (strncmp(filename+j-4, jpg, 4) == 0)
    {
      file_type = 'j';
    }
  else if (strncmp(filename+j-4, txt, 4) == 0)
    {
      file_type = 't';
    }
  else if (strncmp(filename+j-4, png, 4) == 0)
    {
      file_type = 'p';
    }
  else if (strncmp(filename+j-5, html, 5) == 0)
    {
      file_type = 'h';
    }
  return file_type;
}

void create_HTTP_header(char* filename, char filetype, int file_size, char* response)
{
  char content_type[100];
  char content_length[100];
  char cr_nl[3] = {'\r', '\n', '\0'};
  
  memset(response, '\0', 1024);
  strcpy(response, "HTTP/1.1 200 OK");
  strcat(response, cr_nl);
  
  memset(content_type, '\0', 64);

  switch(filetype)
    {
    case 't':
      strcpy(content_type, "Content-Type: text/plain");
      break;
    case 'h':
      strcpy(content_type, "Content-Type: text/html");
      break;
    case 'j':
      strcpy(content_type, "Content-Type: image/jpeg");
      break;
    case 'p':
      strcpy(content_type, "Content-Type: image/png");
      break;
    default: //filetype = 'n' -> binary data
      strcpy(content_type, "Content-Type: application/octet-stream");
      break;
    }
  
  strcat(response, content_type);
  strcat(response, cr_nl);

  char file_size_num[10];
  memset(file_size_num, '\0', 10);
  sprintf(file_size_num, "%d", file_size);
  
  strcpy(content_length, "Content-Length: ");
  strcat(content_length, file_size_num);
  strcat(response, content_length);

  int res_len = strlen(response);
  response[res_len] = '\r';
  response[res_len+1] = '\n';
  response[res_len+2] = '\r';
  response[res_len+3] = '\n';
  response[res_len+4] = '\0';
}

int main()
{
  int sockfd, io_fd, n;
  long file_size;
  socklen_t clilen;
  char buffer[MAXLINE], filetype;
  struct sockaddr_in cliaddr, servaddr;
  char filename[21];
  char http_response[2000000];
  FILE* fp;
  char file_data_buffer[2000000];
  
  sockfd = setup_TCP_connection();
  
  while(1) /* main accept() loop */
    {
      clilen = sizeof(cliaddr);
      if ((io_fd = accept(sockfd, (struct sockaddr*) &cliaddr,
			   &clilen)) < 0)
	{
	  perror("error: unable to accept socket connection\n");
	  continue;
	}
      
      fprintf(stdout, "server: got connection from %s\n",
	      inet_ntoa(cliaddr.sin_addr));

      /* recieved HTTP request */
      while((n = recv(io_fd, buffer, MAXLINE, 0)) > 0)
	{
	  fprintf(stdout, "Received HTTP request from client: \n\n%s", buffer);

	  filetype = parse_HTTP_request(buffer, filename);

	  if (filetype == 't' || filetype == 'h')
	    {
	      fp = fopen(filename, "r");
	    }
	  else //images or any other kind of data
	    {
	      fp = fopen(filename, "rb"); //open in binary reading format
	    }
	  
	  if (fp == NULL)
	    {
	      perror("error: file not found: ");
	      memset(buffer, '\0', MAXLINE);
	      continue;
	      //exit(1);
	    }
	  
	  fseek(fp, 0, SEEK_END); //correctly gets size of file in bytes
	  file_size = ftell(fp);
	  fseek(fp, 0, SEEK_SET); //set file position back to start of file
	  
	  create_HTTP_header(filename, filetype, file_size, http_response);

	  int header_size = strlen(http_response); //length of the header, including '\r', '\n'.

	  memset(file_data_buffer, '\0', 2000000);
	  int b = 0;

	  b = fread(file_data_buffer, sizeof(char), file_size, fp);
	  //strcat(http_response, file_data_buffer);

	  fprintf(stdout, "Diagnostics: \n");
	  fprintf(stdout, "Header size: %d\n", header_size);
	  fprintf(stdout, "Bytes read (file body size): %i\n", b);
	  fprintf(stdout, "Manually counted size of HTTP response: %ld\n", (header_size + file_size)*sizeof(char));
	  fprintf(stdout, "Sending HTTP Response... Header:\n\n%s", http_response);

	  int bytes_sent = send(io_fd, http_response, header_size*sizeof(char), 0);
	  bytes_sent += send(io_fd, file_data_buffer, file_size*sizeof(char), 0);
	  fprintf(stdout, "Bytes successfully sent: %i (out of %ld)\n\n", bytes_sent,
		  (header_size + file_size)*sizeof(char));
	  
	  memset(buffer, '\0', MAXLINE);
	  //http_response zeroed every time a new header is made
	}
      
      if (n < 0)
	{
	  perror("Read error");
	  exit(1);
	}
      
      close(io_fd);
    }
}
