#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

char send_string[5] = {'S', 'E', 'N', 'D', '\0'};
char recv_string[5] = {'R', 'E', 'C', 'V', '\0'};
char timo_string[8] = {'T', 'I', 'M', 'E', 'O', 'U', 'T', '\0'};
char rtra_string[7] = {'R', 'E', 'S', 'E', 'N', 'D', '\0'};
char synf_string[5] = {' ', 'S', 'Y', 'N', '\0'};
char ackf_string[5] = {' ', 'A', 'C', 'K', '\0'};
char finf_string[5] = {' ', 'F', 'I', 'N', '\0'};
char dupa_string[9] = {' ', 'D', 'U', 'P', '-', 'A', 'C', 'K', '\0'};
char empty_string[1] = {'\0'}; 

uint16_t get_seqnum(const char* message)
{
  uint16_t* seqnum_ptr = (uint16_t*) message;
  return ntohs(*seqnum_ptr);
}

uint16_t get_acknum(const char* message)
{
  uint16_t* acknum_ptr = (uint16_t*) (message + 2);
  return ntohs(*acknum_ptr);
}

uint16_t get_synflag(const char* message)
{
  uint16_t* synflag_ptr = (uint16_t*) (message + 4);
  return ntohs(*synflag_ptr);
}

uint16_t get_ackflag(const char* message)
{
  uint16_t* ackflag_ptr = (uint16_t*) (message + 6);
  return ntohs(*ackflag_ptr);
}

uint16_t get_finflag(const char* message)
{
  uint16_t* finflag_ptr = (uint16_t*) (message + 8);
  return ntohs(*finflag_ptr);
}


void add_header(char* buffer, uint16_t seq_num, uint16_t ack_num,
		uint16_t SYN_flag, uint16_t ACK_flag, uint16_t FIN_flag)
{
  memset(buffer, 0, 12);
  uint16_t* buff_ptr = (uint16_t*) buffer;
  *buff_ptr = htons(seq_num);
  buff_ptr = (uint16_t*) (buffer + 2);
  *buff_ptr = htons(ack_num);
  buff_ptr = (uint16_t*) (buffer + 4);
  *buff_ptr = htons(SYN_flag);
  buff_ptr = (uint16_t*) (buffer + 6);
  *buff_ptr = htons(ACK_flag);
  buff_ptr = (uint16_t*) (buffer + 8);
  *buff_ptr = htons(FIN_flag);
  buff_ptr = (uint16_t*) (buffer + 10);
  *buff_ptr = (uint16_t) 0;
}

void log_message(char mess_type, char* message, int dup_ack)
{
  uint16_t m_seqnum, m_acknum;
  char* log_type = empty_string;
  char* syn_mess = empty_string;
  char* ack_mess = empty_string;
  char* fin_mess = empty_string;

  switch(mess_type)
    {
    case 's': //SEND
      log_type = send_string;
      break;
    case 'r':
      log_type = recv_string;
      break;
    case '2':
      log_type = rtra_string;
      break;
    case 't':
      log_type = timo_string;
      fprintf(stdout, "%s %u\n", log_type, get_seqnum(message));
      return;
      break;      
    default:
      fprintf(stderr, "error: invalid message type passed to 'log_message' function\n");
      exit(1);
    }
  
  m_seqnum = get_seqnum(message);
  m_acknum = get_acknum(message);
  
  if (get_synflag(message) == 1)
    {
      syn_mess = synf_string;
    }
  if (get_ackflag(message) == 1)
    {
      ack_mess = ackf_string;
    }
  if (get_finflag(message) == 1)
    {
      fin_mess = finf_string;
    }
  
  if (dup_ack == 1)
    {
      ack_mess = fin_mess;
      fin_mess = dupa_string;
    }
  
  fprintf(stdout, "%s %u %u%s%s%s\n", log_type, m_seqnum, m_acknum, syn_mess, ack_mess, fin_mess); 
}

void request_handler(int sockfd, struct sockaddr* cliaddr, uint16_t* seqnum_init,
		     uint16_t* s_seqnum, char* first_message_buffer, size_t* first_payload_size)
{
  int len, n;
  len = sizeof(*cliaddr);
  uint16_t m_seqnum, rand_seqnum;
  char response[12];
  char buffer[524];
  clock_t send_time;
  double time_since_synack_sent;
  
  n = recvfrom(sockfd, (char *) buffer, 524, MSG_WAITALL,
	       (struct sockaddr *) cliaddr, &len);
  if (n < 0)

    {
      fprintf(stderr, "error: unable to read from socket\n");
      exit(1);
    }
  
  m_seqnum = get_seqnum(buffer);
  //m_acknum = get_acknum(buffer); 
  rand_seqnum = rand() % 25601;
  *s_seqnum = rand_seqnum + 1;

  log_message('r', buffer, 0);
  
  add_header(response, rand_seqnum, (m_seqnum + 1) % 25601, 1, 1, 0);
  
  if (sendto(sockfd, (const char *) response, 12,
	     0, (const struct sockaddr*) cliaddr, len) < 0)
    {
      fprintf(stderr, "error sending SYNACK to client\nerror: %s\n", strerror(errno));
      exit(1);
    }
  send_time = clock();
  log_message('s', response, 0);
  //*seqnum_init = (m_seqnum + 1) % 25601;
  
  while(1)
    {
      n = recvfrom(sockfd, (char *) buffer, 524, MSG_DONTWAIT,
	       (struct sockaddr *) cliaddr, &len);
      if ((n == -1 && (errno != EAGAIN) && (errno != EWOULDBLOCK)) || n == 0)
	{
	  fprintf(stderr, "error reading data from socket\n");
	  exit(1);
	}
      else if (n > 0) //received data from client
	{
	  log_message('r', buffer, 0);
	  if (get_ackflag(buffer) == 1) //wait until synack is ACKed
	    {
	      memcpy(first_message_buffer, buffer, n);
	      *first_payload_size = n - 12;
	      *seqnum_init = get_seqnum(buffer);
	      
	      return;
	    }
	}
      time_since_synack_sent = (double) (clock() - send_time) / CLOCKS_PER_SEC;

      if (time_since_synack_sent > 0.5) //retransmit SYNACK
	{
	  log_message('t', response, 0);
	  if (sendto(sockfd, (const char *) response, 12,
	     0, (const struct sockaddr*) cliaddr, len) < 0)
	    {
	      fprintf(stderr, "error resending SYNACK to client\nerror: %s\n", strerror(errno));
	      exit(1);
	    }
	  send_time = clock(); 
	  log_message('2', response, 0);
	}
      
    }
    
}

int setup_socket(struct sockaddr_in *serv_addr, struct sockaddr *cli_addr, char** argv)
{
  int sockfd_tmp;
  sockfd_tmp = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd_tmp < 0) //setup server-side UDP socket
  {
    fprintf(stderr, "unable to create socket. error %s\n", strerror(errno));
    exit(1);
  }

  memset(serv_addr, 0, sizeof(*serv_addr));
  memset(cli_addr, 0, sizeof(*cli_addr));

  serv_addr->sin_family = AF_INET;
  serv_addr->sin_addr.s_addr = INADDR_ANY;
  serv_addr->sin_port = htons(atoi(argv[1]));

  if (bind(sockfd_tmp, (const struct sockaddr *)serv_addr,
	   sizeof(*serv_addr)) < 0)
    {
      fprintf(stderr, "unable to bind socket\n");
      exit(1);
    }
  
  return sockfd_tmp;
}


int main(int argc, char** argv)
{
  int sockfd;
  char buffer[1024];
  char response_buffer[12];
  struct sockaddr_in servaddr;
  struct sockaddr cliaddr;
  srand(time(0));
  
  if (argc != 2)
    {
      fprintf(stderr, "error: incorrect agruments. Must be of form ./server <PORTNUM>\n");
      exit(1);
    }

  sockfd = setup_socket(&servaddr, &cliaddr, argv);
  
  int len;
  len = sizeof(cliaddr);
  int filenum = 1;
  char filename[10];
  char extension[6] = {'.', 'f', 'i', 'l', 'e', '\0'};
  FILE* fp;
  uint16_t* buf_ptr;
  uint16_t m_seqnum, m_acknum, m_finbit; //n;
  int n; //size_t n;
  uint16_t s_seqnum;
  uint16_t window_min, window_max, expected_seqnum;
  size_t first_payload_size;
  
  while(1)
    {
      request_handler(sockfd, &cliaddr, &window_min, &s_seqnum, buffer, &first_payload_size);

      memset(filename, 0, 10);
      sprintf(filename, "%d%s", filenum, extension);
      
      fp = fopen(filename, "w");
      if (fp == NULL)
	{
	  fprintf(stderr, "error occured opening file\n");
	  exit(1);
	}
      fseek(fp, 0, SEEK_SET);

      if (fwrite(buffer + 12, sizeof(char), first_payload_size, fp) != first_payload_size)
	{
	  fprintf(stderr, "error writing first packet to file: %s\n", strerror(errno));
	  exit(1);
	}

      window_min = (window_min + first_payload_size) % 25601; //first payload guarenteed 
      window_max = (window_min + (10 * 512)) % 25601;         //to have arrived at server at this point

      add_header(response_buffer, s_seqnum, window_min, 0, 1, 0);

      /* ACK 3rd message in 3-way handshake */
      if ( sendto(sockfd, (const char *) response_buffer, 12,
			      0, (const struct sockaddr *) &cliaddr, len) < 0)
	{
	  fprintf(stderr, "error sending DUP ACK %u to client\nerror: %s\n",
		  s_seqnum, strerror(errno));
	  exit(1);
	}
      
      log_message('s', response_buffer, 0);
      
      while(1)
	{
	  n = recvfrom(sockfd, (char *) buffer, 524, MSG_DONTWAIT,
		       (struct sockaddr *) &cliaddr, &len);
	  
	  if ((n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || n == 0)
	    {
	      fprintf(stderr, "error reading from socket\n");
	      exit(1);
	    }
	  
	  else if (n > 0) //packet received from client
	    {
	      m_seqnum = get_seqnum(buffer);
	      log_message('r', buffer, 0);

	      
	      /* send duplicate ACK: didn't receive expected seqnum packet */
	      /*  note: window_min is expected sequence number             */
	      if (m_seqnum != window_min) 
		{
		  //printf("DUP-ACK seqnum: %u\n", m_seqnum);
	      
		  add_header(response_buffer, s_seqnum, window_min, 0, 1, 0);
		  if ( sendto(sockfd, (const char *) response_buffer, 12,
			      0, (const struct sockaddr *) &cliaddr, len) < 0)
		    {
		      fprintf(stderr, "error sending DUP ACK %u to client\nerror: %s\n",
			      s_seqnum, strerror(errno));
		      exit(1);
		    }
		  
		  log_message('s', response_buffer, 1);
		}
	      
	      else /* expected seqnum or FIN packet received */
		{
		  if (get_finflag(buffer) == 1)
		    {
		      /* server has written all data and recieved FIN at this point */

		      char finack_buffer[12];
		      add_header(finack_buffer, s_seqnum, (get_seqnum(buffer) + 1) % 25601,
				 0, 1, 0);
		      
		      if ( sendto(sockfd, (const char *) finack_buffer, 12,
			      0, (const struct sockaddr *) &cliaddr, len) < 0)
			{
			  fprintf(stderr, "error sending FINACK to client\n");
			  exit(1);
			}
		      log_message('s', finack_buffer, 0);
		      window_min = (window_min + 1) % 25601; //no need to update window_max
		      break;
		    }
		   
 		  if(fwrite(buffer + 12, sizeof(char), n - 12, fp) != n - 12)
		    {
		      fprintf(stderr, "error writing to file: %s\n", strerror(errno));
		      exit(1);
		    }
		  window_min = (window_min + n - 12) % 25601;
		  window_max = (window_min + (10 * 512)) % 25601;
		  add_header(response_buffer, s_seqnum, window_min, 0, 1, 0);

		  if ( sendto(sockfd, (const char *) response_buffer, 12,
			      0, (const struct sockaddr *) &cliaddr, len) < 0)
		    {
		      fprintf(stderr, "error sending ACK %u to client\nerror: %s\n",
			      s_seqnum, strerror(errno));
		      exit(1);
		    }
		  log_message('s', response_buffer, 0);
		}
	 
	    }
	  
	}

      char fin_message_buffer[12];
      add_header(fin_message_buffer, s_seqnum, 0, 0, 0, 1);
      
      if ( sendto(sockfd, (const char *) fin_message_buffer, 12,
		  0, (const struct sockaddr *) &cliaddr, len) < 0)
	{
	  fprintf(stderr, "error sending FIN to client\nerror: %s\n", strerror(errno));
	  exit(1);
	}
      
      clock_t time_fin_sent = clock();
      log_message('s', fin_message_buffer, 0);
      double time_since_fin_sent;
      int retransmit_count = 0;
      
      while(1)
	{
	  time_since_fin_sent = (double) (clock() - time_fin_sent) / CLOCKS_PER_SEC;
	  n = recvfrom(sockfd, (char *) buffer, 524, MSG_DONTWAIT, (struct sockaddr *) &cliaddr, &len);

	  if ((n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
	    {
	      fprintf(stderr, "error reading from socket following FIN\n");
	      exit(1);
	    }
	  else if (n > 0)
	    {
	      if (get_acknum(buffer) == (s_seqnum + 1) % 25601 &&
		  get_ackflag(buffer) == 1)
		{
		  /* FIN recieved */
		  log_message('r', buffer, 0);
		  break;
		}
	    }

	  if (time_since_fin_sent > 0.5) //Resend FIN packets
	    {
	      if (retransmit_count == 12) //after 12 retransmissions, I assume
		{                         //client simply shut down, and all of 
		  break;                  //clients final ACKs (for server's FIN)
		                          //were lost. Note that with 30% transmission
		}                         //failure, there is ~5.3 x 10^(-5) percent 
	                                  //chance that all 10 of server's FINs were dropped
	      
	      if ( sendto(sockfd, (const char *) fin_message_buffer, 12,
		  0, (const struct sockaddr *) &cliaddr, len) < 0)
		{
		  fprintf(stderr, "error sending FIN to client\nerror: %s\n", strerror(errno));
		  exit(1);
		}
	      time_fin_sent = clock();
	      log_message('2', fin_message_buffer, 0);
	      retransmit_count += 1;
	    }
	}
      
      
      if (fclose(fp) != 0)
	{
	  fprintf(stderr, "error closing file\n");
	  exit(1);
	}

      filenum += 1;
      
    }
  
}
