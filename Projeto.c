#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h> //Não sei se podemos usar este
#include <sys/socket.h> 
#include <fcntl.h>
#include <pthread.h>

#define K 10 //Têm de ser retirado do ficheiro de config depois
//#define Min 0 //Minimo para as funções random, têm de ser retirado do ficheiro em versões posteriores
//#define Max 255 //Máximo para as funções random, o nosso objetivo é obter valores ASCII válidos, dai 255 como máximo, mesma situação do Min
//Têmos de criar uma struct para a MIB

typedef int IntegerOid; //Para existir a aproximação máxima à escrita de uma MIB definimos estas variáveis.
typedef char *OctetStringOid;


/*Não mexer mais nesta parte até termos o protrocolo de tratamento de requests definindo
typedef struct {
  IntegerOid oid;
  char *name;
  int syntax;
  int maxAccess;
  int status;
  char *description;
} ObjectType;

// Struct para a Tabela
typedef struct {
  IntegerOid keyId;
  OctetStringOid keyValue;
  OctetStringOid keyRequester;
  IntegerOid keyExpirationDate;
  IntegerOid keyExpirationTime;
  IntegerOid keyVisibility;
} DataTableGeneratedKeysEntry;
*/

typedef struct{
    int K_T; //Basicamente K
    int T; //Tempo entre atualizações
} dados_KeyAgent;

unsigned char* Gen_Chave_Mestra(int tamanho_chave){
    int ltime = time(NULL);
    long stime = (unsigned) ltime/2;
    srand(stime);
    char* chave_formada = (char*)malloc(sizeof(char) * (tamanho_chave));
    chave_formada[tamanho_chave] = '\0';

    for(int i = 0; i < tamanho_chave; i ++){
        chave_formada[i]=(char)('0' + rand() % 10);
        //printf("Chave:%c\n",chave_formada[i]);
    }
    return chave_formada;
}

unsigned char* rotate(char* M, int rotation_number){
    
    size_t size= strlen(M);
    char* temp_Return = (char*)malloc(size+1);
    memmove(temp_Return,M,size);
    char temp_array[size];
    temp_array[size] = '\0';
    //printf("Rotation:%d",rotation_number);
    //printf("Chave_M:%s\n",temp_Return);
    /*for(int n = 0;n<rotation_number;n++){
        for (int i = strlen(M) - 1; i > 0; i--) {
            M[i] = M[i - 1];
        }
    }*/
     // Copy the last `n` elements of the original array to the temporary array.
    memmove(temp_array, temp_Return + size - rotation_number, rotation_number*sizeof(char));

  // Copy the remaining elements of the original array to the temporary array.
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number*sizeof(char));

  // Copy the rotated array from the temporary array back to the original array.
    memmove(temp_Return, temp_array, size);
    //printf("M: %s\n",M);
    return temp_Return;
}

unsigned char xor(char Za,char Zb,unsigned char Zc,unsigned char Zd){
    //printf("Za:%c,Zb:%c,Zc:%c,Zd:%c",Za,Zb,Zc,Zd);
    unsigned char value_to_return = (Za^ Zb ^ Zc ^ Zd); 
    return value_to_return;
}

unsigned char random_char(char seed,int inc, int max){
    //Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    //E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    unsigned char Pos_random=(char)((char) inc + rand() % max);
    //printf("Seed usada:%c",seed);
    //printf("Valor random obtido: %c\n",Pos_random);
    //printf("Pos_random gerado = %d\n",Pos_random);
    return Pos_random;
}

unsigned char* random_charZ(unsigned char seed,int inc, int max){
    //Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    //E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    unsigned char Pos_random=(char)((char) inc + rand() % max);
    //printf("Seed usada:%c",seed);
    //printf("Valor random obtido: %c\n",Pos_random);
    //printf("Pos_random gerado = %d\n",Pos_random);
    return Pos_random;
}

unsigned char* rotateZ(unsigned char* M, int rotation_number) {
    size_t size = sizeof(M) / sizeof(M[0]); //Number of bytes / Numero de caracteres 
    unsigned char* temp_Return = (unsigned char*)malloc(size + 1);
    memmove(temp_Return, M, size);
    unsigned char temp_array[size];
    temp_array[size] = '\0';
    // Copy the last n elements of the original array to the temporary array.
    memmove(temp_array, temp_Return + size - rotation_number, rotation_number * sizeof(char));
    // Copy the remaining elements of the original array to the temporary array.
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number * sizeof(char));
    // Copy the rotated array from the temporary array back to the original array.
    memmove(temp_Return, temp_array, size);

    return temp_Return;
}

//Definição das funções para a threads.

void KeyAgent(void *arg){

    dados_KeyAgent *dados = (dados_KeyAgent *)arg;
    int K_T = dados->K_T;
    int T = dados->T;
    int Min = 0;
    int Max = 255;

    //Geração das matrizes e inicialização do keyagent    
    unsigned char Za[K_T][K_T];
    unsigned char Zb[K_T][K_T];
    unsigned char Zc[K_T][K_T];
    unsigned char Zd[K_T][K_T];
    unsigned char Z[K_T][K_T];
    
    for(int contador_row=0;contador_row != K_T;contador_row++){
        for(int contador_collumn=0;contador_collumn != K_T;contador_collumn++){
            //printf("Teste_NUm:%d",contador_collumn);
            Za[contador_row][contador_collumn] = '\0';
            Zb[contador_row][contador_collumn] = '\0';
            Zc[contador_row][contador_collumn] = '\0';
            Zd[contador_row][contador_collumn] = '\0';
            Z[contador_row][contador_collumn] = '\0';
        }
    }

    unsigned char* Chave_Mestra = Gen_Chave_Mestra(2*K_T); //Continuamos a devolver como array ou como int?

    unsigned char Chave_M1[K_T];
    unsigned char Chave_M2[K_T];
    Chave_M1[K_T] = '\0';
    Chave_M2[K_T] = '\0';

    memcpy(Chave_M1,Chave_Mestra,10);
    memcpy(Chave_M2,Chave_Mestra+10,10);

    //Preenchimento das Matrizes Za & Zb
    int contador_row = 0;
    int contador_collumn = 0;
    
    //Za -> Filled per Row

    for(int contador_row=0;contador_row < K;contador_row++){
        char temp_m[K_T];
        temp_m[K_T] = '\0';
        memmove(temp_m,Chave_M1,sizeof(temp_m));
        char *rotated_array = rotate(temp_m,contador_row);
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            if(contador_row == 0){
                Za[contador_row][contador_collumn] = Chave_M1[contador_collumn];
            }else{
                Za[contador_row][contador_collumn] = rotated_array[contador_collumn];
            }
        }
        printf("%s\n",Za[contador_row]);
    }
    
    //Zb -> Filled per Collumn
    for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
        char temp_m2[K_T];
        temp_m2[K_T] = '\0';
        memmove(temp_m2,Chave_M2,sizeof(temp_m2));
        char *rotated_array2 = rotate(temp_m2,contador_collumn);
        for(int contador_row=0;contador_row < K;contador_row++){
            if(contador_collumn == 0){
                Zb[contador_row][contador_collumn] = Chave_M2[contador_row];
            }else{
                Zb[contador_row][contador_collumn] = rotated_array2[contador_row];
            }
        }
    }

    //Zc & Zd -> Filled by random char generated trough the random() function, this random char is calculated from a random number with the original Za or Zb character
    //Of the current position in the matrix as a seed.

    for(int contador_row=0;contador_row < K;contador_row++){
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            Zc[contador_row][contador_collumn] = random_char(Za[contador_row][contador_collumn],Min,Max); //Estamos a ficar com chars a mais no array
            Zd[contador_row][contador_collumn] = random_char(Zb[contador_row][contador_collumn],Min,Max);
        }
    }

    //Z matrix -> Filled by xored elements from all the previous Matrixes. 
    //Procurar como gerar ou perguntar ao professor se devemos tentar usar true random numbers em vez de PRNG do C. Podemos usar aplicações nativas do Unix ou Windows
    //Mas não estou a ver como podemos usar uma seed dessa maneira visto que até agora não encontrei aplicações dessas funções com uma seed
    for(int contador_row=0;contador_row < K;contador_row++){
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            unsigned char temp = xor(Za[contador_row][contador_collumn],
            Zb[contador_row][contador_collumn],
            Zc[contador_row][contador_collumn],
            Zd[contador_row][contador_collumn]);
            Z[contador_row][contador_collumn] = temp;
            printf("Z gerado: %c\n",Z[contador_row][contador_collumn]);
        }
    }

    //Obtenção do tempo
    struct timeval tv;
    int Time_Initilized_sec = tv.tv_sec;
    int N_atualizados = 0; //Número de atualizações da tabela
    // -----------------
    while(1){
        if(tv.tv_sec-Time_Initilized_sec>30){
            // Process and update Z matrix
            // Rotate each row a random number
            for (int contador_row = 0; contador_row < K_T; contador_row++) {
                int rotation_number = random_char(Z[contador_row][0], 0, K_T - 1);
                printf("rotaiton_number: %d\n", rotation_number);
                unsigned char* temp_row = rotateZ(Z[contador_row], rotation_number);
                for (int contador_collumn = 0; contador_collumn < K_T; contador_collumn++) {
                    Z[contador_row][contador_collumn] = temp_row[contador_collumn];
                }
            }

            for (int contador_column = 0; contador_column < K_T; contador_column++) {
                int rotation_number = random_char(Z[0][contador_column], 0, K_T - 1);
                unsigned char* column = malloc(K_T * sizeof(char));
                for (int i = 0; i < K_T; i++) {
                    column[i] = Z[0][i];
                }

                unsigned char* temp_row = rotateZ(column, rotation_number);
                //   printf("here\n");
                for (int contador_row = 0; contador_row < K_T; contador_row++) {
                    Z[contador_row][contador_column] = temp_row[contador_row];
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {

    //Identificadores para cada Thread
    pthread_t KeyAgent_ID;
    pthread_t CommManager_ID;
    pthread_t ProtocolMod_ID;
    //--------------------------------
    
    //Tempo retirado com a inicilização do programa (Time_Since_Initiliaze)
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int Time_Initilized_sec = tv.tv_sec; // seconds
    long int Time_Initilized_microsec = tv.tv_usec; // microseconds
    //---------------------------------------------------------------------

    //Ficheiro de Configuração, Campos:
    //-> Porta UDP usada, Adresso servidor, Adresso Cliente , Tamanho de K, Min e Max caracteres possíveis para a função random,
    //-> De quantos em quantos segundos as tabelas são atualizadas
    int config_fd = 0;
    int Port_Server = 0;
    char Ip_Address_temp[15];
    memset(Ip_Address_temp,'\0',sizeof(Ip_Address_temp));
    size_t K_T = K; //Ao que parece se for realizada qualquer operação sobre um valor do #define o mesmo fica alterado permanentemente
    int Min = 0;
    int Max = 255;
    int T = 0; //Intervalo de tempo entre atualizações
    int Start_up_time = 0; //Tempo desde o início do programa

    if(argc<2){ //Abertura de ficheiro default
        config_fd = open("config_SNMPKeys.txt", O_RDONLY);
    }else{ //Abertura de um ficheiro custom, se quisermos usar isto têmos de defenir um plano que deve ser seguido para que a leitura do ficheiro seja sempre respeitada
        config_fd = open(argv[1],O_RDONLY);
    }
    
    if(config_fd == -1){
        perror("Erro relacionado com o ficheiro de conversão");
        return -1;
    }
    
    char buffer[1024];
    int n = 0;

    while((n=read(config_fd,buffer,sizeof(buffer)))>0){ //Continua a haver coisas a ler no ficheiro de config
        int contador = 0;
        char *temp_ptr;
        char *token = strtok(buffer,",");
        while(token != NULL){
        switch (contador)
        {
        case 0:
            Port_Server = atoi(token);
            break;
        case 1:
            memcpy(Ip_Address_temp,token,strlen(token));
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
        token = strtok(NULL,",");
        contador++;
        }
    }

    //Tratamento extra ligado ao endereço Ip.
    char Ip_Address[strlen(Ip_Address_temp)+1];
    memmove(Ip_Address,Ip_Address_temp,sizeof(Ip_Address));
    Ip_Address[strlen(Ip_Address_temp)+1] == '\0';

    close(config_fd);
    //---------------------------------------------------------------------

    //Inicilização do Servidor

    int Udp_Server_Socket_fd = (AF_INET, SOCK_DGRAM, 0); //Descriptor da socket UDP, AF_INET especifica que vai trabalhar sobre IpV4
    //-------------------------

    //Inicialização keyAgent e Struct para passagem de argumentos
    dados_KeyAgent args_keyagent;
    args_keyagent.K_T = K_T;
    args_keyagent.T = T;

    pthread_create(KeyAgent_ID,NULL,KeyAgent,&args_keyagent);

    return 0;
}