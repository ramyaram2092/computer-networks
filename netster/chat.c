#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>

struct clientDetails
{
  int socketfileDesctiptor;
  char *host;
  long port;
  int serverSocket;
} cd;

void server_udp(char *iface, long port);
void server_tcp(char *iface, long port);
void client_tcp(char *host, long port);
void client_udp(char *host, long port);
// const char * get_ip(char * host, long port);

char *inet_ntoa(struct in_addr in);

int getaddrinfo(const char *restrict node,
                const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res);
const char *inet_ntop(int af, const void *restrict src,
                      char *restrict dst, socklen_t size);
void *serverchatHandler(void *);
void clientchatHandler(int socketFileDescriptor);

void *printServer(void *);

/*
 *  Here is the starting point for your netster part.1 definitions. Add the
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */
void chat_server(char *iface, long port, int use_udp)
{
  if (use_udp == 0)
  {

    server_tcp(iface, port);
  }
  else
  {

    server_udp(iface,port);

  }
}

void chat_client(char *host, long port, int use_udp)
{
  if (use_udp == 0)
  {
    client_tcp(host, port);
  }
  else
  {
    client_udp(host, port);
  }
}



/**
 * @brief Upd server
 *
 * @param iface
 * @param port
 */

void server_udp(char *iface, long port)
{
  // Create a socket
  int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (serverSocket < 0)
  {
    printf("UDP: Problem creating socket \n");
    exit(0);
  }

  struct sockaddr_in server, client;
  memset(&server, 0, sizeof(server));
  memset(&client, 0, sizeof(client));

 // resolve address

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;

  struct addrinfo *response;
  response = (struct addrinfo *)malloc(sizeof(struct addrinfo));
  char str[256];
  sprintf(str, "%ld", port);
  getaddrinfo(iface, str, &hints, &response);
  struct addrinfo *iterator = response;
  char buffer[4096];
  void *raw_addr;

  struct sockaddr_in *tmp = (struct sockaddr_in *)iterator->ai_addr;
  raw_addr = &(tmp->sin_addr);
  inet_ntop(AF_INET, raw_addr, buffer, 4096);


  // assign ip and port
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(buffer);

  // bind socket with server
  if ((bind(serverSocket, (struct sockaddr *)&server, sizeof(server))) != 0)
  {
    printf("UDP : Error in Socket binding \n ");
    exit(0);
  }

  socklen_t clientSize = sizeof(client);
  char clientmsg[256];
  char ip[200];
  bzero(clientmsg, sizeof(clientmsg));

  // chat handler begins
  for (;;)
  {
    // receive client message
    int flag = recvfrom(serverSocket, clientmsg, 256, MSG_WAITALL, (struct sockaddr *)&client, &clientSize);
    if (flag < 0)
    {
      printf("UDP: Error occured while receiving the message \n ");
      return;
    }
    bzero(ip, sizeof(ip));
    inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, 200);
    printf("Got message ('%s',%d)\n",ip,client.sin_port);

    int len = (int)strlen(clientmsg) - 1;
    // convert the recieved message into uppercase
    char client_msg[256];
    int j = 0;
    while (clientmsg[j]!='\n')
    {
      char ch = toupper(clientmsg[j]);
      client_msg[j++] = ch;
    }
    client_msg[j] = '\0';



    // based on the message recieved decide the next course of action
    // case 1:
    if (strncmp(client_msg, "EXIT", len) == 0)
    {
      flag = sendto(serverSocket, "ok\n", 256, 0, (const struct sockaddr *)&client, clientSize);
      if (flag < 0)
      {
        printf("UDP:Error occured while sending the message \n");
        exit(0);
      }
      exit(0);
    }
    
    // case 2:
    else if (strncmp(client_msg, "HELLO", len) == 0)
    {
      // printf("going here");
      flag = sendto(serverSocket, "world\n", 256, 0, (const struct sockaddr *)&client, clientSize);
      if (flag < 0)
      {
        printf("UDP:Error occured while sending the message  \n");
        exit(0);
      }
    }

    // case 3:
    else if (strncmp(client_msg, "GOODBYE", len) == 0)
    {
      flag = sendto(serverSocket, "farewell\n", 256, 0, (const struct sockaddr *)&client, clientSize);
      if (flag < 0)
      {
        printf("UDP:Error occured while sending the message  \n");
        exit(0);
      }
    }

   

    // case 4: send back the recieved message
    else
    {
      flag = sendto(serverSocket, clientmsg, 256, 0, (const struct sockaddr *)&client, clientSize);
      if (flag < 0)
      {
        printf("UDP:Error occured while sending the message\n");
        exit(0);
      }
    }
   bzero(clientmsg, sizeof(clientmsg));


}
}

void client_udp(char *host, long port)
{
  int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientSocket < 0)
  {
    printf("UDP: Error in socket creation at client\n");
    return;
  }
  struct sockaddr_in server;
  socklen_t serverSize = sizeof(server);
  memset(&server, 0, sizeof(server));

  // resolve address

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;

  struct addrinfo *response;
  response = (struct addrinfo *)malloc(sizeof(struct addrinfo));
  char str[256];
  sprintf(str, "%ld", port);
  getaddrinfo(host, str, &hints, &response);
  struct addrinfo *iterator = response;
  char buffer[4096];
  void *raw_addr;

  struct sockaddr_in *tmp = (struct sockaddr_in *)iterator->ai_addr;
  raw_addr = &(tmp->sin_addr);
  inet_ntop(AF_INET, raw_addr, buffer, 4096);


  // set server's ip and host
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(buffer);

  char clientmsg[256];
  char servermsg[256];
  for (;;)
  {

    // get client input
    int i = 0;
    while ((clientmsg[i++] = getchar()) != '\n')
      ;
    clientmsg[i] = '\0';

    // strncat(clientmsg,"\n",1);

    // send the message to server
    int flag = sendto(clientSocket, clientmsg, 256, 0, (const struct sockaddr *)&server, serverSize);
    if (flag < 0)
    {
      printf("UDP:Unable to send message to the server\n ");
      return;
    }

    // Reveive message from the server  sizeof(servermsg),
    flag = recvfrom(clientSocket, servermsg, 256, MSG_WAITALL, (struct sockaddr *)&server, &serverSize);
    if (flag < 0)
    {
      printf("UDP:Problem in receiving server message\n");
      return;
    }
    printf("%s",servermsg);

    int len = (int)strlen(servermsg) - 1;

    // based on the message recieved decide the next course of action

    if ((strncmp(servermsg, "farewell", len) == 0) || (strncmp(servermsg, "ok", len) == 0))
    {
      break;
    }

    // reset clientmsg & servermsg arrays
    bzero(clientmsg, sizeof(clientmsg));
    bzero(servermsg, sizeof(servermsg));
  }
  close(clientSocket);
}

void server_tcp(char *iface, long port)
{

  int serverSocket, newSocket;
  struct sockaddr_in server, client;

  // create socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0)
  {
    printf("TCP: Server socket creation failed \n");
  }

  // resolve address

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;

  struct addrinfo *response;
  response = (struct addrinfo *)malloc(sizeof(struct addrinfo));
  char str[256];
  sprintf(str, "%ld", port);
  getaddrinfo(iface, str, &hints, &response);
  struct addrinfo *iterator = response;
  char buffer[4096];
  void *raw_addr;

  struct sockaddr_in *tmp = (struct sockaddr_in *)iterator->ai_addr;
  raw_addr = &(tmp->sin_addr);
  inet_ntop(AF_INET, raw_addr, buffer, 4096);

  // assign ip and port

  server.sin_family = AF_INET;         // address family IPV4 or 6
  server.sin_addr.s_addr = inet_addr(buffer); 
  server.sin_port =htons(port);

  // bind the socket with the server ip and port
  if ((bind(serverSocket, (struct sockaddr *)&server, sizeof(server))) != 0)
  {
    printf("TCP: Error in socket binding \n ");
    exit(0);
  }

  int i = 0;
  for (;;)
  {
    // listen to the socket connection
    if (listen(serverSocket, 3) != 0) // the no 5 is subjected to change. no of requests that can be queued
    {
      printf("TCP : Listening failed\n ");
      exit(0);
    }

    socklen_t clientLen = sizeof(client);

    // accept the incoming packet from client

    newSocket = accept(serverSocket, (struct sockaddr *)&client, &clientLen);

    if (newSocket < 0)
    {
      printf("TCP: Unable to accept the client packet\n ");
      exit(0);
    }
    char buffer[200];
    inet_ntop(AF_INET, &client.sin_addr.s_addr, buffer, 200);
    printf("Connection %d from (%s,%d)\n", i, buffer, client.sin_port);
    pthread_t id;

    struct clientDetails cd;
    cd.host = buffer;
    cd.port = port;
    cd.socketfileDesctiptor = newSocket;
    cd.serverSocket = serverSocket;

    // handle the chat with client
    pthread_create(&id, NULL, serverchatHandler, &cd);

    i++;
  }

  // close the socket
  close(serverSocket);
}

/**
 * @brief server's chat handler function
 *
 * @param socketFileDescriptor
 */

void *serverchatHandler(void *argp)
{
  struct clientDetails *c = (struct clientDetails *)argp;
  char *host = c->host;
  long port = c->port;
  int socketFileDescriptor = c->socketfileDesctiptor;
  char message[200];
  bzero(message, sizeof(message));

  for (;;)
  {

    // recieve message if any
    if ((recv(socketFileDescriptor, message, sizeof(message), 0) < 0))
    {
      printf("TCP: Coundnt recieve message from client\n");
      exit(0);
    }

    // display the recieved message
    printf("Got message from (%s,%ld)\n", host, port);

    int len = (int)strlen(message) - 1;

    // convert the recieved message to upper case
    char client_msg[200];
    int j = 0;
    while (message[j]!='\n')
    {
      char ch = toupper(message[j]);
      client_msg[j++] = ch;
    }
    client_msg[j] = '\0';

    // case 1: if the client sends "exit", send ok and  server should exit
    if ((strncmp(client_msg, "EXIT", len)) == 0)
    {
      if ((send(socketFileDescriptor, "ok\n", strlen("ok\n"), 0)) < 0)
      {
        printf("TCP: Sending message from server failed\n");
        exit(0);
      }
      exit(0);
    }

    // case 2: if client sends "goodbye"  send farewell and disconnect from the client
    else if ((strncmp(client_msg, "GOODBYE", len)) == 0)
    {
      if ((send(socketFileDescriptor, "farewell\n", strlen("farewell\n"), 0)) < 0)
      {
        printf("TCP: Sending message from server failed\n");
        exit(0);
      }
      break;
    }
    // case 3: if the client sends "hello" respond with world

     else if (strncmp(client_msg, "HELLO", len) == 0)
      {
        if (send(socketFileDescriptor, "world\n", strlen("world\n"), 0) < 0)
        {
          printf("TCP: Sending message from server failed\n");
          exit(0);
        }
      }

    else
    {
        if ((send(socketFileDescriptor, message, strlen(message), 0)) < 0)
        {
          printf("TCP: Sending message from server failed\n");
          exit(0);
        }
      
    }
    bzero(message, sizeof(message));

  }
  fflush(stdout);

  return NULL;
}

/**
 * @brief tcp client
 *
 * @param host
 * @param port
 */
void client_tcp(char *host, long port)
{
  int clientSocket;
  struct sockaddr_in serveraddr;

  // create socket
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0)
  {
    printf("TCP: Client socket creation failed \n");
    exit(0);
  }

  // resolve address

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  struct addrinfo *response;
  response = (struct addrinfo *)malloc(sizeof(struct addrinfo));
  char str[256];
  sprintf(str, "%ld", port);
  getaddrinfo(host, str, &hints, &response);
  struct addrinfo *iterator = response;
  char buffer[4096];
  void *raw_addr;

  struct sockaddr_in *tmp = (struct sockaddr_in *)iterator->ai_addr;
  raw_addr = &(tmp->sin_addr);
  inet_ntop(AF_INET, raw_addr, buffer, 4096);
  // printf("IPv4 %s\n", buffer);

  // assign ip and port
  serveraddr.sin_family = AF_INET; // address family IPV4 or 6
  serveraddr.sin_addr.s_addr = inet_addr(buffer);
  serveraddr.sin_port = htons(port);

  // connect client socket with  server socket
  if (connect(clientSocket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0)
  {
    printf("TCP: Connection with server : %s  and port %s failed\n", buffer,str);
    exit(0);
  }

  // Try to send a message to server
  clientchatHandler(clientSocket);
  close(clientSocket);
}

/**
 * @brief client's chat handler
 *
 * @param socketFileDescriptor
 */

void clientchatHandler(int socketFileDescriptor)
{
  char message[100]; 
  bzero(message, sizeof(message));

  for (;;)
  {

    // read input from user
    int i = 0;
    while ((message[i++] = getchar()) != '\n')
      ;
    message[i] = '\0';
    // strncat(message,"\n",1);

    // send the message
    if (send(socketFileDescriptor, message, strlen(message), 0) < 0)
    {
      printf("TCP: Sending message from client failed\n");
      exit(0);
    }
    bzero(message, sizeof(message));

    // recieve message if any
    if ((recv(socketFileDescriptor, message, sizeof(message), 0) < 0))
    {
      printf("TCP: Recieving message from Server failed \n");
      exit(0);
    }

    printf("%s", message);
    int len = (int)strlen(message) - 1;

    // based on the message recieved decide the next action
    if ((strncmp(message, "farewell", len) == 0) || (strncmp(message, "ok", len) == 0))
    {
      // break;
       exit(0);
    }
    bzero(message, sizeof(message));

  }
 
}
