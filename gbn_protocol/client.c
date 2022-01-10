#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <time.h>

char send_string[5] = {'S', 'E', 'N', 'D', '\0'};
char recv_string[5] = {'R', 'E', 'C', 'V', '\0'};
char timo_string[8] = {'T', 'I', 'M', 'E', 'O', 'U', 'T', '\0'};
char rtra_string[7] = {'R', 'E', 'S', 'E', 'N', 'D' ,'\0'};
char synf_string[5] = {' ', 'S', 'Y', 'N', '\0'};
char ackf_string[5] = {' ', 'A', 'C', 'K', '\0'};
char finf_string[5] = {' ', 'F', 'I', 'N', '\0'};
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
  uint16_t* buff_pointer = (uint16_t*) buffer;
  *buff_pointer = htons(seq_num);
  buff_pointer = (uint16_t*) (buffer + 2);
  *(buff_pointer) = htons(ack_num);
  buff_pointer = (uint16_t*) (buffer + 4);
  *(buff_pointer) = htons(SYN_flag);
  buff_pointer = (uint16_t*) (buffer + 6);
  *(buff_pointer) = htons(ACK_flag);
  buff_pointer = (uint16_t*) (buffer + 8);
  *(buff_pointer) = htons(FIN_flag);
  buff_pointer = (uint16_t*) (buffer + 10);
  *(buff_pointer) = (uint16_t) 0;
}

void log_message(char mess_type, char* message)
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
    case 'r': //RECV
      log_type = recv_string;
      break;
    case '2': //RESEND
      log_type = rtra_string;
      break;
    case 't': //TIMEOUT
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
  
  fprintf(stdout, "%s %u %u%s%s%s\n", log_type, m_seqnum, m_acknum, syn_mess, ack_mess, fin_mess);  
}

void send_SYN(int sockfd, struct sockaddr_in *servaddr, uint16_t* seqnum, uint16_t* acknum)
{
  uint16_t seqnum_init, m_synflag, m_ackflag, m_seqnum, m_acknum;
  int n, len;
  char syn_message[12];
  char response_buffer[12];
  clock_t send_time;
  double time_since_send;
  
  srand(time(0));
  seqnum_init = rand() % 25601;

  add_header(syn_message, seqnum_init, 0, 1, 0, 0);

  if ( sendto(sockfd, (const char *) syn_message, 12,
	 0, (const struct sockaddr *) servaddr,
	      sizeof(*servaddr)) < 0)
    {
      fprintf(stderr, "unable to send SYN message\n");
      exit(1);
    }
  send_time = clock();
  time_since_send = 0;

  log_message('s', syn_message);
  len = sizeof(*servaddr);
  
  while (1)
    {
      time_since_send = (double) (clock() - send_time) / CLOCKS_PER_SEC;
      
      n = recvfrom(sockfd, (char *) response_buffer, 12,
		   MSG_DONTWAIT, (struct sockaddr *) servaddr,
		   &len);
      if (n == -1 && (errno != EAGAIN) && (errno != EWOULDBLOCK)) //actual error occured
	{
	  fprintf(stderr, "error occured reading from socket: %s\n", strerror(errno));
	  exit(1);
	}
      
      else if (n == -1 && time_since_send > 0.5) //no data received, timeout
	{
	  //retransmit SYN
	  if ( sendto(sockfd, (const char *) syn_message, 12,
		      0, (const struct sockaddr *) servaddr,
		      sizeof(*servaddr)) < 0)
	    {
	      fprintf(stderr, "unable to send SYN message\n");
	      exit(1);
	    }
	  send_time = clock();
	  log_message('t', syn_message);
	  log_message('2', syn_message);
	}
      
      else //received server data
	{
	  if (n == 0)
	    {
	      fprintf(stderr, "server shutdown\n");
	      exit(1);
	    }
	  if (get_synflag(response_buffer) == 1 && get_ackflag(response_buffer) == 1)
	    {
	      //SYN ACK received
	      break;
	    }
	}
    }
  
  *seqnum = get_seqnum(response_buffer); 
  *acknum = get_acknum(response_buffer); 
  log_message('r', response_buffer);
}

int setup_socket(struct sockaddr_in *serv_addr, char** argv)
{
  int sockfd_tmp;
  struct hostent* he;
  
  if ( (sockfd_tmp = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
      fprintf(stderr, "error: unable to create socket\n");
      exit(1);
    }

  memset(serv_addr, 0, sizeof(*serv_addr));
  
  serv_addr->sin_family = AF_INET;
  serv_addr->sin_port = htons(atoi(argv[2]));
  
  if ( (he = gethostbyname(argv[1])) == NULL )
    {
      serv_addr->sin_addr.s_addr = inet_addr(argv[1]);
    }
  else
    {
      memcpy(&(serv_addr->sin_addr), he->h_addr_list[0], he->h_length);
    }
  
  return sockfd_tmp;
}


int main(int argc, char** argv)
{
  int sockfd;
  struct sockaddr_in servaddr;

  if (argc != 4)
    {
      fprintf(stderr, "error: improper number of command line arguments. Must be of form:\n ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>");
      exit(1);
    }

  sockfd = setup_socket(&servaddr, argv);
  
  uint16_t synack_seq_num, synack_ack_num;
  
  send_SYN(sockfd, &servaddr, &synack_seq_num, &synack_ack_num); //this will give seqnum, acknum
                                                                 //that server returns in SYNACK message
  FILE* fp;
  size_t file_size;
  
  if ((fp = fopen(argv[3], "r")) == NULL)
    {
      fprintf(stderr, "error: unable to open specified file\n");
      exit(1);
    }

  if (fseek(fp, 0, SEEK_END) != 0)
    {
      fprintf(stderr, "error: unable to seek EOF\n");
      exit(1);
    }

  file_size = ftell(fp);
  
  if (fseek(fp, 0, SEEK_SET) != 0) //set file position to beginning of file
    {
      fprintf(stderr, "error: unable to seek beginning of file\n");
      exit(1);
    }

  char packet_buffer[10][524];
  char server_data_buffer[12]; //524
  size_t read_size, data_read, data_sent, data_acked;
  int n, len;

  uint16_t m_acknum, m_seqnum, seq_num;
  
  seq_num = synack_ack_num;
  uint16_t fin_bit;

  data_read = 0;
  data_sent = 0;
  data_acked = 0;
  
  int open_buff_index = 0;
 
  size_t payload_size[10];

  uint16_t window_min = seq_num;
  uint16_t window_max = (seq_num + (10*512)) % 25601;
  uint16_t next_unbuffed_seqnum = seq_num;
  uint16_t next_send_seqnum = seq_num;
  int next_send_index = 0;
  int window_min_index = 0; //highest_acknum_packet_index
  uint16_t s_ack_bit = 1;
  uint16_t s_ack_num = synack_seq_num + 1; //ack_num;
  int highest_buffered_packet_index = 0;
  clock_t send_times[10];
  double time_since_oldest_packet_sent;
  int packets_to_resend;
    
  fin_bit = 0;
  
  while(1)
    {
      /* buffer up to 10 packets (of 512 bytes) worth of file data */
      while( (next_unbuffed_seqnum < window_max ||
	      ((window_max < window_min) && (next_unbuffed_seqnum >= window_min)) )
	    && data_read < file_size)
	{
	  read_size = fread(packet_buffer[open_buff_index] + 12, 1, 512, fp);
	  add_header(packet_buffer[open_buff_index], next_unbuffed_seqnum, s_ack_num, 0, s_ack_bit, 0);
	  highest_buffered_packet_index = open_buff_index;

	  data_read += read_size;
	  s_ack_bit = 0;
	  s_ack_num = 0;
	  payload_size[open_buff_index] = read_size;

	  if (read_size < 0)
	    {
	      fprintf(stderr, "error reading file data\n");
	      exit(1);
	    }

	  next_unbuffed_seqnum = (next_unbuffed_seqnum + read_size) % 25601;
	  open_buff_index = (open_buff_index + 1) % 10;
	}

    
      /* send packets for 1st time */
      while( (next_send_seqnum < window_max ||
	      ((window_max < window_min) && (next_send_seqnum >= window_min)) )
	      && data_sent < file_size ) //&& data_acked != file_size 
	{                                                          
	  /*if (payload_size[next_send_index] == 0) //removing this broke everything earlier,
	    {                                     //don't touch it
	      break;
	      }*/
	  
	  if ( sendto(sockfd, (const char *) packet_buffer[next_send_index],
		      (payload_size[next_send_index]) + 12, 0,
		      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	    {
	      fprintf(stderr, "unable to send packet %u\n", get_seqnum(packet_buffer[next_send_index]));
	      exit(1);
	    }
	  
	  send_times[next_send_index] = clock();
	  log_message('s', packet_buffer[next_send_index]);

	  data_sent += payload_size[next_send_index];
	  next_send_seqnum = (next_send_seqnum + payload_size[next_send_index]) % 25601;
	  next_send_index = (next_send_index + 1) % 10;      
	}
      

      /* resend all buffered packets if timeout occurs */
      time_since_oldest_packet_sent = (double) (clock() - send_times[window_min_index]) / CLOCKS_PER_SEC; 
      if (time_since_oldest_packet_sent > 0.5)
	{
	  log_message('t', packet_buffer[window_min_index]);
	  
	  packets_to_resend = highest_buffered_packet_index - window_min_index + 1;
	  
	  if (window_min_index > highest_buffered_packet_index) //wraps over
	    {
	      packets_to_resend = (10 - window_min_index) + (highest_buffered_packet_index + 1);
	    }
	  
	  for (int i = 0; i < packets_to_resend; i++)
	    {
	      if ( sendto(sockfd, (const char *) packet_buffer[(window_min_index + i) % 10],
			  (payload_size[(window_min_index + i) % 10]) + 12, 0,
		      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		{
		  fprintf(stderr, "unable to resend packet %u\n",
			  get_seqnum(packet_buffer[(window_min_index + i) % 10]));
		  exit(1);
		}
	      send_times[(window_min_index + i) % 10] = clock();
	      log_message('2', packet_buffer[(window_min_index + i) % 10]);
	    }
	  
	}

      
      /* receive message from server */
      len = sizeof(servaddr);
      n = recvfrom(sockfd, (char *) server_data_buffer, 12, //524
	       MSG_DONTWAIT, (struct sockaddr *) &servaddr,
		   &len); 
      if ((n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) || n == 0)
	{
	  fprintf(stderr, "error reading incoming ACK statements\n");
	  exit(1);
	}
      if (n == -1) //no server data
	{
	  continue;
	}
      	
      log_message('r', server_data_buffer);
      
      m_acknum = get_acknum(server_data_buffer); 
      
      /* Cumulative ACK implementation */
      if ((m_acknum > window_min) || (window_max < window_min) && (m_acknum <= window_max))
	{
	  if (m_acknum > window_min)
	    {
	      data_acked += (size_t) (m_acknum - window_min);
	    }
	  else
	    {
	      data_acked += (size_t) (25601 - window_min + m_acknum);
	    }
	  
	  window_min = m_acknum;
	  for (int i = 0; i < 10; i++)
	    {
	      if (get_seqnum(packet_buffer[i]) == m_acknum)
		{
		  window_min_index = i;
		}
	    }

	  window_max = (window_min + (10*512)) % 25601;
	}
      
      //(1) if (window_min == ((file_size + seq_num) % 25601)) 
      //above implementation (1) not fully correct, since for large files you can
      //wrap fully around range of seqnums, and have window_min be equal to the value
      //without of (file_size + seq_num) % 25601 depending on initial seq_num and file_size. 
      
      if (data_acked == file_size) //all data ACKed (not including FIN)
	{                 
	  break;
	}

  }

  char fin_buffer[12];
  add_header(fin_buffer, (file_size + seq_num) % 25601, 0, 0, 0, 1);
  if ( sendto(sockfd, (const char *) fin_buffer, 12, 0,
	      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
      fprintf(stderr, "unable to send FIN\n");
      exit(1);
    }
  clock_t fin_time_sent = clock();
  clock_t shutdown_timer;
  
  log_message('s', fin_buffer);
  int finack_recieved = 0;
  double delay;
  
  //len = sizeof(servaddr);
  while(1)
    {
      n = recvfrom(sockfd, (char *) server_data_buffer, 12, //524
		   MSG_DONTWAIT, (struct sockaddr *) &servaddr,
		   &len);
      if ((n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || n == 0)
	{
	  fprintf(stderr, "error reading FIN ACK packet\n");
	  exit(1);
	}
      
      else if (n > 0)
	{
	  log_message('r', server_data_buffer);
	  if (get_ackflag(server_data_buffer) == 1 &&
	      get_acknum(server_data_buffer) == (file_size + seq_num + 1) % 25601)
	    {
	      finack_recieved = 1;
	    }
	  else if (get_finflag(server_data_buffer) == 1)
	    {
	      shutdown_timer = clock();
	      delay = 0;
	      m_seqnum = get_seqnum(server_data_buffer);
	      break;
	    }
	}
	
      delay = (double) (clock() - fin_time_sent) / CLOCKS_PER_SEC;
      if (delay > 0.5) /* Retransmit FIN */
	{
	  if ( sendto(sockfd, (const char *) fin_buffer, 12, 0,
		      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	    {
	      fprintf(stderr, "unable to resend FIN\n");
	      exit(1);
	    }
	  fin_time_sent = clock();
	  log_message('2', fin_buffer);
	}
    }

  char server_finack_buffer[12];
  add_header(server_finack_buffer, (file_size + seq_num + 1) % 25601,
	     (m_seqnum + 1) % 25601, 0, 1, 0);
  
  if ( sendto(sockfd, (const char *) server_finack_buffer, 12, 0,
		      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	    {
	      fprintf(stderr, "unable to send FIN\n");
	      exit(1);
	    }
  log_message('s', server_finack_buffer);
  delay = (double) (clock() - shutdown_timer) / CLOCKS_PER_SEC;
  
  while(delay < 2)
    {
      delay = (double) (clock() - shutdown_timer) / CLOCKS_PER_SEC;
  
      n = recvfrom(sockfd, (char *) server_data_buffer, 12, //524
		   MSG_DONTWAIT, (struct sockaddr *) &servaddr,
		   &len);
      if ((n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) || n == 0)
	{
	  fprintf(stderr, "error reading FIN ACK packet\n");
	  exit(1);
	}
      if (n > 0 && get_finflag(server_data_buffer) == 1) //FIN ACK lost, resend
	{
	  if ( sendto(sockfd, (const char *) server_finack_buffer, 12, 0,
		      (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	    {
	      fprintf(stderr, "unable to resend FIN\n");
	      exit(1);
	    }
	  log_message('2', server_finack_buffer);
	}    
    }
  
  close(sockfd);
  fclose(fp);
}
