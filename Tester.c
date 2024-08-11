#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h> 
#include <fcntl.h>

#define SERVER_PORT 8080
#define SECURITY_PARAMETERS "0|0|0"


int random_request_num_gen(){

  srand(time(NULL));
  int randomNumber = rand() % 999999999 + 100000000;

  return randomNumber;
}

int control_generated_IDS(int new_ID){

  char assisting_buffer[20], *check_result;
  assisting_buffer[19] = '\0';
  char assisting_buffer_epoch[11];
  char assisting_buffer_RID[10];
  assisting_buffer[9] = '\0';
  assisting_buffer_epoch[10] = 0;
  int repeated_ID = new_ID;

  rename("RequestIDs.txt","RequestIDsOLD.txt");
  FILE *id_file_old = fopen("RequestIDsOLD.txt","r+");
  int fd = open("RequestIDs.txt",O_CREAT,0666);
  if(!id_file_old || fd == 0){
    printf("Couldn't access the Request ID file");
    exit(0);
  }
  close(fd);

  FILE *id_file_new = fopen("RequestIDs.txt","w");
  if(!id_file_new){
    printf("Couldn't access the Request ID file");
    exit(0);
  }

  while((fgets(assisting_buffer,sizeof(assisting_buffer),id_file_old)) != NULL){
    if(assisting_buffer[0] == NULL){
      break;
    }
    memmove(assisting_buffer_RID,assisting_buffer,9);
    memmove(assisting_buffer_epoch,assisting_buffer+9,10);
    if(atoi(assisting_buffer) == new_ID){
      printf("This ID is already in the file,generate a new one");
      repeated_ID = 1;
    }
    time_t check_epoch_time = (time_t)atoi(assisting_buffer_epoch);
    if(time(NULL) - check_epoch_time >15){
      //This ID has no more TTL, we won't rewrite it;
    }else{
      if(repeated_ID != 1){
        fprintf(id_file_new,"%s",assisting_buffer);
      }
    }
  }
  if(repeated_ID!= 1){
    fprintf(id_file_new,"%d%ld\n",new_ID,time(NULL));
  }
  fclose(id_file_new);
  fclose(id_file_old);
  remove("RequestIDsOLD.txt");
  return repeated_ID;
}

int contaDigitos(int num) {

    int count = 0;

    if (num == 0) {
        return 1;
    }

    if (num < 0) {
        num = -num;
    }

    while (num != 0) {
        num /= 10;
        ++count;
    }

    return count;
}

void print_menu(){
  printf("Gestor V.1\n");
  printf("1-> Get\n");
  printf("2-> Set\n");
  printf("0-> Exit\n");
  printf("Escolha uma das opções: ");
}

void decompile_agent_answer(char *message_buffer){
  printf("*Answer from the SNMPkey Agent*\n");
  int end_of_message = 0;
  int PDU_container_counter = 0;
  char *substring_token;
  size_t full_message_size = strlen(message_buffer);
  message_buffer[full_message_size+1] = '\0';
  substring_token = strtok(message_buffer, "|");
  
  while(end_of_message != 1){
      while(substring_token != NULL){
          if(PDU_container_counter == 5){
              printf("Number of Answers without errors:%s\n",substring_token);
          }
          if(PDU_container_counter == 6){
              for(int print_assistant = 0; print_assistant<strlen(substring_token); print_assistant++){
                if(substring_token[print_assistant]!='(' && substring_token[print_assistant]!=')'){
                  printf("I-ID / OID Accessed -> ");
                  while(substring_token[print_assistant] != ';'){
                    printf("%c",substring_token[print_assistant]);
                    if(substring_token[print_assistant] == ','){
                      printf(" | Result: ");
                    }
                    print_assistant++;
                  }
                  printf("\n");
                }
              }
          }
          if(PDU_container_counter == 7){
              printf("Number of Answers with errors:%s\n",substring_token);
          }
          if(PDU_container_counter == 8){
              for(int print_assistant = 0; print_assistant<strlen(substring_token); print_assistant++){
                if(substring_token[print_assistant]!='(' && substring_token[print_assistant]!=')'){
                  printf("I-ID / OID Accessed -> ");
                  while(substring_token[print_assistant] != ';'){
                    if(substring_token[print_assistant] == ','){
                      printf(" | Result: ");
                    }else{
                      printf("%c",substring_token[print_assistant]);
                    }
                    print_assistant++;
                  }
                  printf("\n");
                }
              }
          }
          PDU_container_counter++;
          substring_token = strtok(NULL, "|");
      }
      end_of_message = 1;
  }
  free(substring_token);
}

int main(int argc, char *argv[]) {
  
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in server_addr,client_addr;
  socklen_t server_len = sizeof(server_addr);
  struct timeval timeout;
  char buffer[1024];
  int bytes_sent, bytes_received;
  int menu_option = -1;
  int chosen_op = 4;
  int num_pedidos = 0;
  int num_erros = 0;
  int ID_pedido = 0;
  char *mensagem;
  char *ip_origem = "127.1.1.1";
  char *ip_destino = "127.0.0.1";
  char int_to_char_helper[2]; //Simple int to char conversion
  char random_ID[10];
  char *manager_ID;
  //PDU -> 0,0,0,Número até 9 dígitos,0||1||2, Número de pedidos,Lista estilo (N do pedido, dados ; N do pedido, dados), Número de erros (Igual ao de pedidos) , Lista de Erros (N do erro, dados)***

  mensagem = (char*)malloc(100*sizeof(char)); //Começamos com um espaço de 100 bytes, se estes forem ocupados, fazemos realloc. Caso a mensagem seja menor que 100, o mesmo acontece
  
  if(argc>1){
    //We can force an ID for testing reasons
    //We have a valid ID, otherwise random_ID remains as a null filled array until we give it a random ID
    manager_ID = (char *)malloc(strlen(argv[1])*sizeof(char)+1);
    strcat(manager_ID,argv[1]);
  }

  if (sockfd < 0) {
    perror("socket");
    exit(1);
  }


  //Informação do servidor
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip_destino);
  server_addr.sin_port = htons(SERVER_PORT);

  //Info para a configuração da socket do cliente
  struct sockaddr_in localAddress;
  socklen_t addrLength = sizeof(localAddress);

  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set receiv timeout");
        close(sockfd);
        exit(EXIT_FAILURE);
  }

  int new_id = 0;

  while(new_id == 0){
    new_id = random_request_num_gen();
    new_id = control_generated_IDS(new_id);
  }
  
  printf("Request ID will be->%d\n",new_id);

  print_menu();
  while(menu_option == -1){
    scanf(" %d",&menu_option);
    if(menu_option == 1){
        chosen_op = 0;
        int occupied_buffer_space = 0;
        printf("Para a realização de um get deve indicar primeiro quantos gets vai realizar e de seguida o I-ID e o valor para o mesmo (I-ID,Value)\n");
        printf("Quantos pedidos quer realizar:");
        scanf("%d\n",&num_pedidos);
        char *buffer;
        buffer = (char *)malloc(300*sizeof(char));
        strcat(buffer,"(");
        for(int n = 0; n != num_pedidos; n++){
          char input[256];
          if (fgets(input, sizeof(input), stdin) != NULL){
            size_t length = strlen(input);
            if (length > 0 && input[length - 1] == '\n') {
                input[length - 1] = '\0';
            }
            strcat(buffer,input);
          }
          if(n+1 != num_pedidos){
            strcat(buffer,";");
          }
        }
        strcat(buffer,";)");

        size_t total_buffer_length = strlen(buffer)+1;
        buffer = realloc(buffer, total_buffer_length*sizeof(char)); // Fazemos isto para basicamente poupar espaço e tornar a mensagem a enviar num datagrama mais limpo
        buffer[total_buffer_length] = '\0';
        /*Formulação da mensagem final a enviar*/
        strcat(mensagem,SECURITY_PARAMETERS);
        strcat(mensagem,"|");
        strcat(mensagem,manager_ID);
        strcat(mensagem,"|");
        if(random_ID[0] == NULL){
          sprintf(random_ID,"%d",new_id);
        }
        strcat(mensagem,random_ID);
        strcat(mensagem,"|");
        sprintf(int_to_char_helper,"%d",menu_option);
        strcat(mensagem,int_to_char_helper);
        strcat(mensagem,"|");
        int temp_digits = contaDigitos(num_pedidos);
        char temp_digit_buffer[temp_digits+1];
        sprintf(temp_digit_buffer,"%d",num_pedidos);
        strcat(mensagem,temp_digit_buffer);
        strcat(mensagem,"|");
        strcat(mensagem,buffer);
        strcat(mensagem,"|");

        size_t total_length_buffer = strlen(mensagem)+1;
        mensagem = realloc(mensagem, total_length_buffer*sizeof(char));
        mensagem[strlen(mensagem)+1] = '\0';

        //Não sei se têmos de incluir os erros já neste PDU

    }if(menu_option == 2){
        chosen_op = 1;
        int occupied_buffer_space = 0;
        printf("Para a realização de um set deve indicar primeiro quantos gets vai realizar e de seguida o I-ID e o valor para o mesmo (I-ID,Value)\n");
        printf("Quantos pedidos quer realizar:");
        scanf("%d\n",&num_pedidos);
        char *buffer;
        buffer = (char *)malloc(300*sizeof(char));
        strcat(buffer,"(");
        for(int n = 0; n != num_pedidos; n++){
          char input[256];
          if (fgets(input, sizeof(input), stdin) != NULL){
            size_t length = strlen(input);
            if (length > 0 && input[length - 1] == '\n') {
                input[length - 1] = '\0';
            }
            strcat(buffer,input);
          }
          if(n+1 != num_pedidos){
            strcat(buffer,";");
          }
        }

        strcat(buffer,";)");
        size_t total_buffer_length = strlen(buffer)+1;
        buffer = realloc(buffer, total_buffer_length*sizeof(char)); // Fazemos isto para basicamente poupar espaço e tornar a mensagem a enviar num datagrama mais limpo
        buffer[total_buffer_length] = '\0';
        /*Formulação da mensagem final a enviar*/
        strcat(mensagem,SECURITY_PARAMETERS);
        strcat(mensagem,"|");
        strcat(mensagem,manager_ID);
        strcat(mensagem,"|");
        if(random_ID[0] == NULL){
          sprintf(random_ID,"%d",new_id);
        }
        strcat(mensagem,random_ID);
        strcat(mensagem,"|");
        sprintf(int_to_char_helper,"%d",menu_option);
        strcat(mensagem,int_to_char_helper);
        strcat(mensagem,"|");
        int temp_digits = contaDigitos(num_pedidos);
        char temp_digit_buffer[temp_digits+1];
        sprintf(temp_digit_buffer,"%d",num_pedidos);
        strcat(mensagem,temp_digit_buffer);
        strcat(mensagem,"|");
        strcat(mensagem,buffer);
        strcat(mensagem,"|");

        size_t total_length_buffer = strlen(mensagem)+1;
        mensagem = realloc(mensagem, total_length_buffer*sizeof(char));
        mensagem[strlen(mensagem)+1] = '\0';
        //Não sei se têmos de incluir os erros já neste PDU

    }if(menu_option == 0){
      printf("Closing the agent\n");
      close(sockfd);
      _exit(0);
    }
    if(menu_option<0 || menu_option >2){
      menu_option = -1;
      printf("Nenhuma opção válida escolhida\n");
    }
  }

  //printf("%s",mensagem);

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    exit(1);
  }


  //Send message to server
  //while(1){
  bytes_sent = sendto(sockfd, mensagem, strlen(mensagem), 0,(struct sockaddr *)&server_addr,sizeof(server_addr));
  if (bytes_sent < 0) {
    perror("send");
    exit(1);
  }
  printf("Sent Request->%s\n",mensagem);
  memset(buffer,'\0',1024);
  //}
  // Receive response from server
  socklen_t addr_len = sizeof(client_addr);

  bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&server_addr, &server_len);
  if (bytes_received < 0) {
    perror("Receive failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  // Display the contents of the structure
  printf("Received Answer: %s\n",buffer);

  decompile_agent_answer(buffer);

  close(sockfd);
  free(mensagem);
  //free(buffer);

  printf("\n Closing");

  return 0;
}