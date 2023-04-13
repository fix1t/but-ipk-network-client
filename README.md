# Network client documentation
Program ``ipkcpc.c`` is a command-line application that enables users to connect to a server via TCP or UDP protocols. It was developed to communicate with ``ipkpd`` server.

Application is compatible with both Linux and Windows operating systems and requires user input for the host and port number. To run the network client user must run first build the application using ``make`` in the application's root directory and then execute the binary file it with ``./ipkcpc -h [host] -p [port] -m [mode]``, where mode is either 'tcp' or 'udp'.

After executing the binary file, the client prompts the user to enter a message to be send to the server, sends the message via the chosen protocol, and displays the server's response.

To terminate the communicaction the application listens to ``Ctrl+c`` signal.

The network client is supported on various Linux and Windows operating systems, including NixOS and Windows 10. It has been tested on these platforms, but it should work on other versions as well.

# Theory behind client 
Firstly our client has to provide 4 different routes for user. 

WINDOWS_PLATFORM    -   TCP
                    -   UDP

LINUX_PLATFORM      -   TCP
                    -   UDP

## UDP
UDP is a connectionless protocol that operates at the transport layer of the OSI (Open Systems Interconnection) model. It is simple and lightweight, making it faster than TCP. 
Typical client-site system callings are in this order:
    1 create ``client_socket`` socket
    2 bind to relevant server
    3 send the data to relevat server using ``sendto()`` function
    4 wait for response from server using ``recvfrom()`` function
    5 close the socket

## TCP
TCP is a connection-oriented protocol that also operates at the transport layer of the OSI model. It provides reliable, ordered, and error-checked delivery of data between applications. 
Typical client-site system callings are in this order:
    1 create ``client_socket`` socket
    2 bind to relevant server
    3 connect to the serever - establish communication
    4 send the data to relevat server using ``send()`` function
    5 wait for response from server using ``recvfrom()`` function
    6 close the socket

## Difference between Linux and Windows 

The difference between TCP/UDP on Windows and Linux can manifest in a few ways. 

First, the function calls used to set up a TCP/UDP connection differ slightly between the two operating systems. For example, on Windows, the ``WSAStartup()`` function is used to initialize the Winsock library, while on Linux, this is not necessary. Additionally, some data types, such as ``SOCKET`` on Windows and ``int`` on Linux, are used to represent socket (file) descriptors. 

When it comes to UDP, there is also a difference in ``sendto()`` and ``recvfrom()`` functions. Windows requires the use of the ``socklen_t`` data type to specify the size of the address structure passed to the function, while on Linux, this can be done using the ``socklen_t`` or ``int`` data types.

## Interesting code section explained with comments
ln 230: Binding server
```c
// socket in family -> IPv4
memset((char *) &server_address, '\0', sizeof(server_address));
server_address.sin_family = AF_INET;
memcpy((char*)&server_address.sin_addr, (char*)server->h_addr, server->h_length);
// port to little-endian - network byte order
server_address.sin_port = htons(port_number);
```  

ln 265: different sockets based on protocol and platform
```c
//open socket for TCP
#ifdef _WIN32 //WIDNOWS ONLY type: SOCKET Client_socket
    if ((Client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
#else //LINUX ONLY type: int Client_socket
    if ((Client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
#endif
...
//open socket for UDP
#ifdef _WIN32 //WIDNOWS ONLY type: SOCKET Client_socket
    if ((Client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
#else //LINUX ONLY type: int Client_socket
    if ((Client_socket = socket(AF_INET, SOCK_DGRAM, 0)) <= 0)
#endif
```
  
ln 145: Sending UDP packet. Attachment  of the head
```c
// user message 
if(fgets(buf, BUFSIZE, stdin) == NULL){
    perror("ERROR in fgets");
    exit(EXIT_FAILURE);
}
// header attachment to buffer to create propper package
const int messageSize = strlen(buf)+3; 
char message[BUFSIZE+3];
// OP code -> 0 = request
memset(message, '\0',BUFSIZE+3);
// length of the payload message as 2nd byte
message[1] = strlen(buf);
strcpy(message+2,buf);
// platform dependent type of serverlength
#ifdef _WIN32
    int serverlen = sizeof(server_address);
#else
    socklen_t serverlen = sizeof(server_address);
#endif // _WIN32
// sending [messageSize] bytes of message to server 
int bytestx = sendto(Client_socket, message, messageSize, 0, (const struct sockaddr *) &server_address, serverlen);
```

ln 203: Recieving UDP packet. Packets are fragile and may be lost on the way. Timeout is required
```c
 // Wait for the socket to become ready for reading or until timeout expires
  int select_result = select(Client_socket + 1, &readfds, NULL, NULL, &tv);
  if (select_result == -1) { // error
    ...
  } else if (select_result == 0) { // timeout
    fprintf(stderr,"ERROR: Packet timed out. Have not received any response from the server. Udp packet maybe lost on the way. Try again.\n");
    return udpCommunication(server_address);
  
  } else { // Socket is ready for reading    
    int bytesrx = recvfrom(Client_socket, buf, BUFSIZE, 0, (struct sockaddr *)&server_address, &serverlen);
    ...
  }
```

# Testing

Testing was done mostly by program hearing. Debug prints could be still accessed by changing ``#DEBUG`` macro in the source code.

```sh
        [DEBUG] INFO: Server socket: 127.0.0.1 : 2023 
        [DEBUG] Creating socket ... 
        [DEBUG] UDP communication ...
        [DEBUG] Please enter msg: HELLO
        [DEBUG] Waiting for response ...0
ERROR: Packet timed out. Have not received any response from the server. Udp packet maybe lost on the way. Try again.
        [DEBUG] Please enter msg: (+ 3 1)
        [DEBUG] Waiting for response ...0
ERROR: Packet timed out. Have not received any response from the server. Udp packet maybe lost on the way. Try again.
        [DEBUG] Please enter msg: ^C    
        [DEBUG] Closing socket udp...
```

or

```sh
        [DEBUG] Waiting for response ...0
        [DEBUG] OP CODE : 1     STATUS : 1      PAYLOAD LENGTH : 27 
ERR:Could not parse the message
        [DEBUG] Please enter msg: (+ 2 21)
        [DEBUG] Waiting for response ...0
        [DEBUG] OP CODE : 1     STATUS : 0      PAYLOAD LENGTH : 2 
OK:23
        [DEBUG] Please enter msg: ^C    
        [DEBUG] Closing socket udp...
```

next I have made some static testing with ``ipkcpc_tests.cpp`` testing file. ``cat``-ing input from files and comparing the output.

# Sources

A backbone of the source code was inspired by 'Stbus':  https://git.fit.vutbr.cz/NESFIT/IPK-Projekty/src/branch/master/Stubs and IPK course presentations.