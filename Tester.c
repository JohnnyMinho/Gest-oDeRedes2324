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
  printf("1-> Get\n");
  printf("2-> Set\n");
  printf("0-> Exit\n");
}

int main(void) {
  
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in server_addr;
  char buffer[1024];
  int bytes_sent, bytes_received;
  int menu_option = -1;
  int chosen_op = "4";
  int num_pedidos = 0;
  int num_erros = 0;
  int ID_pedido = 0;
  char *mensagem;
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
    scanf("Escolha uma das opções %d",menu_option);
    switch(menu_option){
      case(1):
        chosen_op = 0;
        int occupied_buffer_space = 0;
        printf("Ao digitar a que componenetes pretende aceder, digite sempre um "","" entre cada");
        scanf("Quantos pedidos quer realizar: %d",num_pedidos);
        char buffer[300];
        
        for(int n = 0; n < num_pedidos; n++){
          char input[100];
          if (fgets(input, sizeof(input), stdin) != NULL){
            // Remove the newline character if present
            size_t length = strlen(input);
            if (length > 0 && input[length - 1] == '\n') {
                input[length - 1] = '\0';
            }
            strcat(buffer,input);
          }
        }
        size_t total_buffer_length = strlen(buffer)
        break;
      case(2):
        chosen_op = 1;
        scanf("Quantos pedidos quer realizar: %d",num_pedidos);
        for(int n = 0; n < num_pedidos; n++){
          
        }
        break;
      case(0):
        printf("Closing the agent");
        _exit(0);
        break;
      default:
        printf("Nenhuma opção válida escolhida");
        break;
    }
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