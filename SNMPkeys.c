#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "Dependencies/hashmap/hashmap.h"
#include "Dependencies/LineReader/LineReader.h"
#include "SNMPkeys.h"

#define TIMEOUT_BIND 60
#define K 10  // Têm de ser retirado do ficheiro de config depois
// #define Min 0 //Minimo para as funções random, têm de ser retirado do ficheiro em versões posteriores
// #define Max 255 //Máximo para as funções random, o nosso objetivo é obter valores ASCII válidos, dai 255 como máximo, mesma situação do Min
// Têmos de criar uma struct para a MIB

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex1_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
//pthread_cond_t cond1_1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

//Precisamos de usar mutex e condições para evitar que os processos bloquem o programa em certos momentos, um momento critíco em que se deve evitar bloquear o programa é na abertura da socket

//int socket_initialized = 0;

sigset_t signal_set;

typedef int IntegerOid;  // Para existir a aproximação máxima à escrita de uma MIB definimos estas variáveis. -> Pode ser substituido por um Int
typedef char *OctetStringOid;

// Struct para a Tabela

int mib_compare(const void *a, const void *b, void *udata) {
    const struct ObjectType *ua = a;
    const struct ObjectType *ub = b;
    char temp_ua[11];
    char temp_ub[11];
    temp_ua[10] == '\0';
    temp_ub[10] == '\0';
    sprintf(temp_ua,"%ls",ua->oid);
    sprintf(temp_ub,"%ls",ub->oid);
    return strcmp(temp_ua, temp_ub);
}

/*bool mib_iter(const void *item, void *udata) {
    ObjectType *mib = item;
    printf("%s (age=%d)\n", mib->name, mib->oid);
    return true;
}*/

uint64_t mib_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct ObjectType *mib = item;
    return hashmap_sip(mib->oid, sizeof(mib->oid), seed0, seed1);
}

unsigned char *Gen_Chave_Mestra(int tamanho_chave) {
    int ltime = time(NULL);
    long stime = (unsigned)ltime / 2;
    srand(stime);
    char *chave_formada = (char *)malloc(sizeof(char) * (tamanho_chave));
    chave_formada[tamanho_chave] = '\0';

    for (int i = 0; i < tamanho_chave; i++) {
        chave_formada[i] = (char)('0' + rand() % 10);
    }
    return chave_formada;
}

unsigned char *rotate(char *M, int rotation_number) {
    size_t size = strlen(M);
    char *temp_Return = (char *)malloc(size + 1);
    memmove(temp_Return, M, size);
    char temp_array[size];
    temp_array[size] = '\0';

    // É optado o uso do memmove para evitar overflows.
    memmove(temp_array, temp_Return + size - rotation_number, rotation_number * sizeof(char));
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number * sizeof(char));
    memmove(temp_Return, temp_array, size);
    return temp_Return;
}

unsigned char xor (char Za, char Zb, unsigned char Zc, unsigned char Zd) {
    unsigned char value_to_return = (Za ^ Zb ^ Zc ^ Zd);
    return value_to_return;
}

unsigned char random_char(char seed, int inc, int max) {
    // Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    // E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    unsigned char Pos_random = (char)((char)inc + rand() % max);
    return Pos_random;
}

unsigned char *random_charZ(unsigned char seed, int inc, int max) {
    // Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    // E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    unsigned char Pos_random = (char)((char)inc + rand() % max);
    return Pos_random;
}

unsigned char *rotateZ(unsigned char *M, int rotation_number) {
    size_t size = sizeof(M) / sizeof(M[0]);  // Number of bytes / Numero de caracteres
    unsigned char *temp_Return = (unsigned char *)malloc(size + 1);
    memmove(temp_Return, M, size);
    unsigned char temp_array[size];
    temp_array[size] = '\0';
    memmove(temp_array, temp_Return + size - rotation_number, rotation_number * sizeof(char));
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number * sizeof(char));
    memmove(temp_Return, temp_array, size);

    return temp_Return;
}

struct Hashmap *MibSetter(int fd){
    //Função que devolve o hashmap com  
}

// Definição das funções para a threads.

// No server usamos a solução sugerida pelo professor que é o uso de um ficheiro com o número de pedidos total

void *ProtocolAgent(void *arg){
    //Só precisamos de um arguemnto a ser recebido, que é o buffer associado ao pedido, estes depois faz tudo desde tratamento dos dados a
    //processamento.
    //1 thread por pedido. Enviamos o resultado para a CommAgent ou fazemos aqui o envio por uma questão de simplicidade/efc?
    
    
}

void *CommAgent(void *arg) {
    //pthread_mutex_lock(&mutex);
    //PDU -> 0,0,0,Número até 9 dígitos,0||1 ||2, Número de pedidos,Lista estilo (N do pedido, dados ; N do pedido, dados), Número de erros (Igual ao de pedidos) , Lista de Erros (N do erro, dados)***
    // Caso o pedido não tenha erros o default é (0,0) para os dados
    // Para nos facilitar a vida, temos uma sequência de 3 * para definir o fim do PDU já que há um tamanho variável de dados

    printf("CommAgent Started\n");
    fd_set read_fds;
    struct timeval timeout;
    int Udp_Server_Socket_fd = socket(AF_INET, SOCK_DGRAM, 0);  // Descriptor da socket UDP, AF_INET especifica que vai trabalhar sobre IpV4
    dados_CommAgent *dados = (dados_CommAgent *)arg;
    char server_message[2000], client_message[2000];
    struct sockaddr_in server;  // Esta socket é mais por questões de facilitar a vida ao usar o inet
    char buffer_numpedidos[11];
    buffer_numpedidos[11] = '\0';
    int fd_rqfile = open("Num_Requests.txt", O_RDWR);
    read(fd_rqfile, buffer_numpedidos, sizeof(buffer_numpedidos));  // Podiamos optar por uma solução mais bonita a partir do fstat

    int num_pedidos = atoi(buffer_numpedidos);
    printf("Address: %s:%d\n", dados->IP_add,dados->port);
    printf("%lc\n", sizeof(dados->IP_add));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(dados->IP_add);
    server.sin_port = htons(dados->port);

    if (server.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address: %s\n", dados->IP_add);
        exit(-1);
    }

    if (Udp_Server_Socket_fd < 0) {
        printf("UDP socket creation wasn't possible, shutting down!");
        exit(-1);  // Podemos trocar por um exit;
    }              // Erro na Criação

    // Basicamente dizer que a socket passa a ter estas propriedades

    //Definição dos tempos de timeout (é tido em conta qualquer ação, ou seja, se a socket estiver mais de 1 minuto sem realizar qualquer ação, o servidor é fechado)
    
    FD_ZERO(&read_fds);
    FD_SET(Udp_Server_Socket_fd, &read_fds);

    timeout.tv_sec = TIMEOUT_BIND;
    timeout.tv_usec = 0;

    // Bind das info da socket a ela 

    while (bind(Udp_Server_Socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        if (errno == EADDRNOTAVAIL) {
            fprintf(stderr, "Error: Ip pode não estar disponível \n");
            exit(-1);
        } else if (errno == EADDRINUSE) {
            fprintf(stderr, "Error: Ip já usado...\n");
            sleep(1);  // Add a delay before retrying
        } else {
            perror("Error binding");
            exit(-1);
        }
    }

    // Verificamos se a socket ficou atribuída
    int len_nome_server = sizeof(server);
    socklen_t length_server = len_nome_server;
    if (getsockname(Udp_Server_Socket_fd, (struct sockaddr *)&server, &len_nome_server) < 0) {
    perror("Error getting socket name");
    exit(-3);
    }
    //pthread_mutex_unlock(&mutex);

    while(1){
        //pthread_mutex_lock(&mutex2);
        int select_result = select(Udp_Server_Socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        char buffer[1024];
        ssize_t num_bytes_waiting = read(Udp_Server_Socket_fd,buffer,sizeof(buffer)-1);
        size_t information_length = strlen(num_bytes_waiting);
        
        if (select_result == 0) {
            fprintf(stderr, "Error: Timed out.\n");
        } else if (select_result < 0) {
            perror("Error: Select malfunction.\n");
        } else {
            printf(stderr, "Error: Adress already in use.\n");
        }
        if (select_result == -1){
            perror(select_result);
        }
        if (select_result != 0 && select_result != -1 ){
            //Têmos de ler o conteúdo do set escolhido para o select(), este buffer deve ter o tamanho do PDU de modo a ler cada pedido
            //Ver se com o select os dados são lidos estilo stack / pilha
        }
        if (num_bytes_waiting > 0){
                //printf("Teste");
                //printf("Buffer->%s",buffer);
                write(0,buffer,sizeof(buffer));
            }
        //pthread_mutex_unlock(&mutex2);
    }

}

void *KeyAgent(void *arg) {
    printf("KeyAgent Started\n");
    dados_KeyAgent *dados = (dados_KeyAgent *)arg;
    int K_T = dados->K_T;
    int T = dados->T;
    int Min = 0;
    int Max = 255;
    
    // Geração das matrizes e inicialização do keyagent
    unsigned char Za[K_T][K_T];
    unsigned char Zb[K_T][K_T]; 
    unsigned char Zc[K_T][K_T];
    unsigned char Zd[K_T][K_T];
    unsigned char Z[K_T][K_T];    

    for (int contador_row = 0; contador_row != K_T; contador_row++) {
        for (int contador_collumn = 0; contador_collumn != K_T; contador_collumn++) {
            // printf("Teste_NUm:%d",contador_collumn);
            Za[contador_row][contador_collumn] = '\0';
            Zb[contador_row][contador_collumn] = '\0';
            Zc[contador_row][contador_collumn] = '\0';
            Zd[contador_row][contador_collumn] = '\0';
            Z[contador_row][contador_collumn] = '\0';
        }
    }

    unsigned char *Chave_Mestra = Gen_Chave_Mestra(2 * K_T);  // Continuamos a devolver como array ou como int?

    unsigned char Chave_M1[K_T];
    unsigned char Chave_M2[K_T];
    Chave_M1[K_T] = '\0';
    Chave_M2[K_T] = '\0';

    memcpy(Chave_M1, Chave_Mestra, K_T);
    memcpy(Chave_M2, Chave_Mestra + K_T, K_T);

    // Preenchimento das Matrizes Za & Zb
    int contador_row = 0;
    int contador_collumn = 0;

    // Za -> Filled per Row
    for (int contador_row = 0; contador_row < K; contador_row++) {
        char temp_m[K_T];
        temp_m[K_T] = '\0';
        memmove(temp_m, Chave_M1, sizeof(temp_m));
        char *rotated_array = rotate(temp_m, contador_row);
        for (int contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            if (contador_row == 0) {
                Za[contador_row][contador_collumn] = Chave_M1[contador_collumn];
            } else {
                Za[contador_row][contador_collumn] = rotated_array[contador_collumn];
            }
        }
        printf("%s\n", Za[contador_row]);
    }

    // Zb -> Filled per Collumn
    for (int contador_collumn = 0; contador_collumn < K; contador_collumn++) {
        char temp_m2[K_T];
        temp_m2[K_T] = '\0';
        memmove(temp_m2, Chave_M2, sizeof(temp_m2));
        char *rotated_array2 = rotate(temp_m2, contador_collumn);
        for (int contador_row = 0; contador_row < K; contador_row++) {
            if (contador_collumn == 0) {
                Zb[contador_row][contador_collumn] = Chave_M2[contador_row];
            } else {
                Zb[contador_row][contador_collumn] = rotated_array2[contador_row];
            }
        }
    }

    // Zc & Zd -> Filled by random char generated trough the random() function, this random char is calculated from a random number with the original Za or Zb character
    // Of the current position in the matrix as a seed.
    for (int contador_row = 0; contador_row < K; contador_row++) {
        for (int contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            Zc[contador_row][contador_collumn] = random_char(Za[contador_row][contador_collumn], Min, Max);  // Estamos a ficar com chars a mais no array
            Zd[contador_row][contador_collumn] = random_char(Zb[contador_row][contador_collumn], Min, Max);
        }
    }

    // Z matrix -> Filled by xored elements from all the previous Matrixes.
    // Procurar como gerar ou perguntar ao professor se devemos tentar usar true random numbers em vez de PRNG do C. Podemos usar aplicações nativas do Unix ou Windows
    // Mas não estou a ver como podemos usar uma seed dessa maneira visto que até agora não encontrei aplicações dessas funções com uma seed
    for (int contador_row = 0; contador_row < K; contador_row++) {
        for (int contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            unsigned char temp = xor(Za[contador_row][contador_collumn],
                                     Zb[contador_row][contador_collumn],
                                     Zc[contador_row][contador_collumn],
                                     Zd[contador_row][contador_collumn]);
            Z[contador_row][contador_collumn] = temp;
            printf("Z gerado: %c\n", Z[contador_row][contador_collumn]);
        }
    }

    int mem_block_size = K_T*K_T;
    //printf("MEM_BLOCK_SIZE => %d",mem_block_size);

    memmove(dados->Z_S,Z, mem_block_size * sizeof(unsigned char));
    
    // Obtenção do tempo
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int Time_Initilized_sec = tv.tv_sec;
    int N_atualizados = 0;  // Número de atualizações da tabela
    printf("Tempo:%d\n", Time_Initilized_sec);
    // -----------------
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond1, &mutex);
            // Process and update Z matrix
            // Rotate each row a random number
            for (int contador_row = 0; contador_row < K_T; contador_row++) {
                int rotation_number = random_char(Z[contador_row][0], 0, K_T - 1);
                printf("rotation_number: %d\n", rotation_number);
                unsigned char *temp_row = rotateZ(Z[contador_row], rotation_number);
                for (int contador_collumn = 0; contador_collumn < K_T; contador_collumn++) {
                    Z[contador_row][contador_collumn] = temp_row[contador_collumn];
                }
            }

            for (int contador_column = 0; contador_column < K_T; contador_column++) {
                int rotation_number = random_char(Z[0][contador_column], 0, K_T - 1);
                unsigned char *column = malloc(K_T * sizeof(char));
                for (int i = 0; i < K_T; i++) {
                    column[i] = Z[0][i];
                }

                unsigned char *temp_row = rotateZ(column, rotation_number);
                for (int contador_row = 0; contador_row < K_T; contador_row++) {
                    Z[contador_row][contador_column] = temp_row[contador_row];
                }
            }
            memmove(dados->Z_S,Z,mem_block_size * sizeof(unsigned char));
            Time_Initilized_sec = tv.tv_sec;
            pthread_mutex_unlock(&mutex);
    }
}

int main(int argc, char *argv[]) {
    // Identificadores para cada Thread
    pthread_t KeyAgent_ID;
    pthread_t CommManager_ID;
    pthread_t ProtocolMod_ID;
    //--------------------------------

    // Guardar dados do servidor para enviar como argumento
    dados_CommAgent args_commagent;

    // Tempo retirado com a inicilização do programa (Time_Since_Initiliaze)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int Time_Initilized_sec = tv.tv_sec;             // seconds
    long int Time_Initilized_microsec = tv.tv_usec;  // microseconds
    //---------------------------------------------------------------------

    // Ficheiro de Configuração, Campos:
    //-> Porta UDP usada, Adresso servidor, Adresso Cliente , Tamanho de K, Min e Max caracteres possíveis para a função random,
    //-> De quantos em quantos segundos as tabelas são atualizadas
    int config_fd = 0;
    int Port_Server = 0;
    char Ip_Address_temp[15];
    memset(Ip_Address_temp, '\0', sizeof(Ip_Address_temp));
    size_t K_T = K;  // Ao que parece se for realizada qualquer operação sobre um valor do #define o mesmo fica alterado permanentemente
    int Min = 0;
    int Max = 255;
    int T = 0;              // Intervalo de tempo entre atualizações
    int Start_up_time = 0;  // Tempo desde o início do programa

    //Variáveis de verificação de inicialização
    int signumber;
    int commport_ready = 0;
    //-----------------------------------------

    if (argc < 2) {  // Abertura de ficheiro default
        config_fd = open("config_SNMPKeys.txt", O_RDONLY);
    } else {  // Abertura de um ficheiro custom, se quisermos usar isto têmos de defenir um plano que deve ser seguido para que a leitura do ficheiro seja sempre respeitada
        config_fd = open(argv[1], O_RDONLY);
    }

    if (config_fd == -1) {
        perror("Erro relacionado com o ficheiro de conversão");
        return -1;
    }

    char buffer[1024];
    int n = 0;

    while ((n = read(config_fd, buffer, sizeof(buffer))) > 0) {  // Continua a haver coisas a ler no ficheiro de config
        int contador = 0;
        char *temp_ptr;
        char *token = strtok(buffer, ",");
        while (token != NULL) {
            switch (contador) {
                case 0:
                    Port_Server = atoi(token);
                    break;
                case 1:
                    memcpy(Ip_Address_temp, token, strlen(token));
                    break;
                case 2:
                    K_T = atoi(token);
                    break;
                case 3:
                    Min = atoi(token);
                    break;
                case 4:
                    Max = atoi(token);
                    break;
                case 5:
                    T = atoi(token);
                    break;
                default:
                    break;
            }
            token = strtok(NULL, ",");
            contador++;
        }
    }

    // Tratamento extra ligado ao endereço Ip.
    char Ip_Address[strlen(Ip_Address_temp) + 1];
    memmove(Ip_Address, Ip_Address_temp, sizeof(Ip_Address));
    Ip_Address[strlen(Ip_Address_temp) + 1] == '\0';

    close(config_fd);
    //---------------------------------------------------------------------

    // Inicilização do Servidor & Definição dos argumentos
    args_commagent.port = Port_Server;
    args_commagent.IP_add = Ip_Address;

    //-------------------------

    // Inicialização keyAgent e Struct para passagem de argumentos
    dados_KeyAgent args_keyagent;
    args_keyagent.K_T = K_T;
    args_keyagent.T = T;
    args_keyagent.Z_S = malloc(K_T*K_T*sizeof(unsigned char));

    unsigned char *temp_z = &args_keyagent.Z_S;

    char buffer_numpedidos[11];
    buffer_numpedidos[11] = '\0';

    struct hashmap *MIB = hashmap_new(sizeof(struct ObjectType),0,0,0,mib_hash,mib_compare, NULL,NULL);

    pthread_create(&CommManager_ID, NULL, CommAgent, &args_commagent);
    pthread_create(&KeyAgent_ID, NULL, KeyAgent, &args_keyagent);
    
    //pthread_join(KeyAgent_ID, NULL);

    gettimeofday(&tv, NULL);
    time_t signal_time_assistant = tv.tv_sec;
    time_t print_ZTable = tv.tv_sec;

    //sigwait(&signal_set, &signumber); //Vamos fazer com que o programa espere que a porta fique 100% operacional antes de avançar

    while(1){
        if (tv.tv_sec - signal_time_assistant > 10){
            pthread_cond_signal(&cond1);
            signal_time_assistant = tv.tv_sec;
        }
        if ((tv.tv_sec - print_ZTable) > 6){
            //printf("Current ZTable => %s\n",args_keyagent.Z_S);
            /*
            for(int test_counter = 0; test_counter <100;test_counter++){
                printf("Tagala:%hhu\n",args_keyagent.Z_S[test_counter]);
            }
            */
            print_ZTable = tv.tv_sec;
        }
        gettimeofday(&tv, NULL);
        //sleep(1);
    }

    return 0;
}