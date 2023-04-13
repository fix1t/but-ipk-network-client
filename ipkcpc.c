#include <stdio.h> // standard input/output functions
#include <stdlib.h> // exit()
#include <string.h> 
#include <sys/types.h> // socket-related functions and data types
#include <stdbool.h>

/* PLATFORM */
#ifdef _WIN32
  #include <winsock2.h>
  #include <Windows.h> //signal handling
  #pragma comment(lib, "ws2_32.lib")
  #define WINDOWS 1
  WSADATA wsa;
  SOCKET Client_socket;
#else
  #include <sys/socket.h>
  #include <netdb.h>
  #include <netinet/in.h> // internet protocol-related functions and data types
  #include <arpa/inet.h> // functions to manipulate IP addresses
  #include <unistd.h> // POSIX operating system API functions
  #include <signal.h> //signal handling
  #define  WINDOWS 0
  int Client_socket;
#endif // _WIN32

/* DEBUG */
#define DEBUG 0
#define DEBUGPRINT(...)     \
  if (DEBUG) {              \
    printf("\t[DEBUG]\t");  \
    printf(__VA_ARGS__);    \
  }                         \

#define BUFSIZE 255

void printUsage(const char *exe){
  fprintf(stderr,"\nusage: %s -h <hostname> -p <port> -m <mode>\n-host is the IPv4 address of the server \n-port the server port\n-mode either tcp or udp \n(e.g., ipkcpc -h 1.2.3.4 -p 2023 -m udp).\n\n", exe);
}

void ctrlcHandlerTcp(int sig){
  DEBUGPRINT("Closing socket tcp...\n");
  int bytestx = send(Client_socket, "BYE\n", 4, 0);
  if (bytestx < 0) {
    perror("ERROR in sendto");
    exit(1);
  }
  
  DEBUGPRINT("Recieving BYE...\n");
  char buf[BUFSIZE];
  memset(buf,'\0', BUFSIZE);
  int bytesrx = recv(Client_socket, buf, BUFSIZE, 0);
  if (bytesrx < 0) {
    perror("ERROR in recvfrom");
    exit(1);
  }

  // Response from server
  printf("%s", buf);
  #ifdef _WIN32
    WSACleanup();
    closesocket(Client_socket);
  #else
    close(Client_socket);
  #endif 
  exit(0);
}

void ctrlcHandlerUdp(int sig){
  DEBUGPRINT("Closing socket udp...\n");
  #ifdef _WIN32
    WSACleanup();
    closesocket(Client_socket);
  #else
    close(Client_socket);
  #endif 
  exit(0);
}


#ifdef _WIN32
  BOOL WINAPI ConsoleHandlerTcp(DWORD ctrlType) {
      if (ctrlType == CTRL_C_EVENT){
      ctrlcHandlerTcp(0);
    }
  }
  BOOL WINAPI ConsoleHandlerUdp(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT){
      ctrlcHandlerUdp(0);
    }
  }
#endif // _WIN32

int tcpCommunication(){
  //watch for interupt
  #ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandlerTcp, TRUE);
  #else
    signal(SIGINT, ctrlcHandlerTcp);
  #endif // _WIN32

  char buf[BUFSIZE];
  // User input
  memset(buf,'\0', BUFSIZE);
  DEBUGPRINT("Please enter msg: \n");
  if(fgets(buf, BUFSIZE, stdin) == NULL){
    perror("ERROR in fgets");
    return 1;
  }

  if (strlen(buf) == BUFSIZE-1)
  {
    printf("ERR: Buffer overflow. Maximum allowed characters is %d.\n",BUFSIZE);
    ctrlcHandlerTcp(0);    
    return 1;
  }

  // Send  
  int bytestx = send(Client_socket, buf, strlen(buf), 0);
  if (bytestx < 0) {
    perror("ERROR in sendto");
    return 1;
  }
  
  // clear buffer
  memset(buf, '\0', BUFSIZE);

  // Read 
  int bytesrx = recv(Client_socket, buf, BUFSIZE, 0);
  if (bytesrx < 0) {
    perror("ERROR in recvfrom");
    return 1;
  }

  // Response from server
  printf("%s", buf);

  // Server shuts the conversation
  if (strcmp("BYE\n", buf)){
    return tcpCommunication();
  }
  
  DEBUGPRINT("Got BYE...\n")
  return 0;
}

int udpCommunication(struct sockaddr_in server_address){
  //watch for interupt
  #ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandlerUdp, TRUE);
  #else
    signal(SIGINT, ctrlcHandlerUdp);
  #endif // _WIN32

  // user message input
  char buf[BUFSIZE];
  memset(buf, '\0', BUFSIZE);
  DEBUGPRINT("Please enter msg: ");
  if(fgets(buf, BUFSIZE, stdin) == NULL){
    perror("ERROR in fgets");
    exit(EXIT_FAILURE);
  }

  // attach header to buffer to create message
  const int messageSize = strlen(buf)+3; 
  char message[BUFSIZE+3];
  memset(message, '\0',BUFSIZE+3);
  message[1] = strlen(buf);
  strcpy(message+2,buf);

  if (strlen(buf) == BUFSIZE-1)
  {
    printf("ERR: Buffer overflow. Maximum allowed characters is %d.\n",BUFSIZE);
    ctrlcHandlerUdp(0);    
    return 1;
  }

  // sendto server
  #ifdef _WIN32
    int serverlen = sizeof(server_address);
  #else
    socklen_t serverlen = sizeof(server_address);
  #endif // _WIN32
  int bytestx = sendto(Client_socket, message, messageSize, 0, (const struct sockaddr *) &server_address, serverlen);
    if (bytestx < 0) 
      perror("ERROR: sendto");

  memset(buf, '\0', BUFSIZE);

  DEBUGPRINT("Waiting for response ...%d\n",buf[0]);

  // Set a timeout of 5 seconds
  struct timeval tv;
  fd_set readfds;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  // Clear the file descriptor set
  FD_ZERO(&readfds);
  // Add the socket to the file descriptor set
  FD_SET(Client_socket, &readfds);

  // Wait for the socket to become ready for reading or until timeout expires
  int select_result = select(Client_socket + 1, &readfds, NULL, NULL, &tv);
  if (select_result == -1) {
    perror("ERROR: recvfrom");
    exit(EXIT_FAILURE);
  } else if (select_result == 0) { //timeout
    fprintf(stderr,"ERROR: Packet timed out. Have not received any response from the server. Udp packet maybe lost on the way. Try again.\n");
    return udpCommunication(server_address);
  
  } else { // Socket is ready for reading    
    int bytesrx = recvfrom(Client_socket, buf, BUFSIZE, 0, (struct sockaddr *)&server_address, &serverlen);
    if (bytesrx < 0) {
      perror("ERROR: recvfrom");
      exit(EXIT_FAILURE);
    }

    // Handle received packet
    // Recieving message format:
    // [OP-code][Status-code][Payload-length][PAYLOAD] ... [PAYLOAD]
    DEBUGPRINT("OP CODE : %d\tSTATUS : %d\tPAYLOAD LENGTH : %d \n", buf[0],buf[1],buf[2]);
    // OP CODE

    if (buf[0] != 1){
      fprintf(stderr,"Response corrupted. Unexpected operation code. Try again.\n");  
      return udpCommunication(server_address);
    }

    // PAYLOAD LENGTH    
    if (buf[2] != (char)strlen(buf+3)){
      fprintf(stderr,"Response corrupted. Unexpected payload size. Try again.\n");  
      return udpCommunication(server_address);
    }

    // STATUS CODE
    if (buf[1] == 1)
      printf("ERR:%s\n",buf+3);
    else if (buf[1] == 0)
      printf("OK:%s\n",buf+3);  
    else { //unexpected op code
      fprintf(stderr,"Response corrupted. Unexpected status code. Try again.\n");
      return udpCommunication(server_address);
    }

    udpCommunication(server_address);
    return 0;
  }


  return(1);
}

int main (int argc, const char * argv[]) 
{
  int port_number;
  bool tcpMode = false;
  const char *server_hostname;
  struct hostent *server;
  struct sockaddr_in server_address;

  // argument parser
  if (argc != 7 || strcmp(argv[1], "-h") || strcmp(argv[3], "-p") || strcmp(argv[5], "-m")) {
    printUsage(argv[0]);
    exit(EXIT_FAILURE);
  }

  // get adress and port
  server_hostname = argv[2];
  port_number = atoi(argv[4]);

  if(!strcmp(argv[6], "udp")){
    tcpMode = false;
  }else if (!strcmp(argv[6], "tcp")){
    tcpMode = true;
  }else{
    printUsage(argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((server = gethostbyname(server_hostname)) == NULL) {
    fprintf(stderr,"ERROR: no such host as %s\n", server_hostname);
    exit(EXIT_FAILURE);
  }

  // socket in family -> IPv4
  memset((char *) &server_address, '\0', sizeof(server_address));
  server_address.sin_family = AF_INET;
  memcpy((char*)&server_address.sin_addr, (char*)server->h_addr, server->h_length);
  // to little-endian - network byte order
  server_address.sin_port = htons(port_number);
 
  #ifdef _WIN32 //WIDNOWS ONLY
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) 
    {
      perror("WSAStartup failed");
      return -1;
    }
  #endif

  DEBUGPRINT("INFO: Server socket: %s : %d \n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
  
  // socket creation 
  DEBUGPRINT("Creating socket ... \n");

  // TCP 
  if (tcpMode){
    //open socket
    #ifdef _WIN32 //WIDNOWS ONLY
      if ((Client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    #else //LINUX
      if ((Client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    #endif
    {
      perror("ERROR: socket");
      exit(EXIT_FAILURE);
    }

    //connect
    if (connect(Client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0)
    {
      perror("ERROR: connect");
      exit(EXIT_FAILURE);        
    }
    DEBUGPRINT("TCP commmunication establiched ...\n")
    tcpCommunication();
  }
  // UDP
  else 
  {
    //open socket
    #ifdef _WIN32 //WIDNOWS ONLY
      if ((Client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
    #else //LINUX
      if ((Client_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
    #endif
    {
      perror("ERROR: socket");
      exit(EXIT_FAILURE);
    }
    DEBUGPRINT("UDP communication ...\n");  
    udpCommunication(server_address);
  }

  return 0;
}