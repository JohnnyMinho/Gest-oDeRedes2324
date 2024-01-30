#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h> 

#define SERVER_PORT 8080
#define SECURITY_PARAMETERS "0,0,0"


int random_request_num_gen(){
  srand(time(NULL));
  int randomNumber = rand() % 999999999 + 100000000;
  return randomNumber;
}

void print_menu(){
  printf("Gestor V.1\n");
  printf("1-> Response\n");
  printf("2-> Get\n");
  printf("3-> Set\n");
  printf("0-> Exit\n");
}

int main(void) {
  
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in server_addr;
  char buffer[1024];
  int bytes_sent, bytes_received;
  int menu_option = -1;

  //PDU -> 0,0,0,Número até 9 dígitos,0||1||2, Número de pedidos,Lista estilo (N do pedido, dados ; N do pedido, dados), Número de erros (Igual ao de pedidos) , Lista de Erros (N do erro, dados)***

  if (sockfd < 0) {
    perror("socket");
    exit(1);
  }

  /* Connect to server */
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(SERVER_PORT);

  print_menu();
  while(menu_option == -1){
    scanf("Escolha uma das opções")
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    exit(1);
  }


  /* Send message to server */
  bytes_sent = send(sockfd, SEC, strlen(MESSAGE), 0);
  if (bytes_sent < 0) {
    perror("send");
    exit(1);
  }

  close(sockfd);

  printf("Closing");

  return 0;
}