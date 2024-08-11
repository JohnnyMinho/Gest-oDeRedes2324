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
#include <ctype.h>
#include "SNMPkeys.h"

#define SET_REQUEST_MAX_ASNWER_SPACE 256 //Maximum amount of space to save the value of a set request
#define TIMEOUT_BIND 60
#define K 10  // Têm de ser retirado do ficheiro de config depois
// #define Min 0 //Minimo para as funções random, têm de ser retirado do ficheiro em versões posteriores
// #define Max 255 //Máximo para as funções random, o nosso objetivo é obter valores ASCII válidos, dai 255 como máximo, mesma situação do Min
// Têmos de criar uma struct para a MIB

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex1_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER; //Mutex for creating the key, avoid multiple input attempts at the same time to keys_table
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
//pthread_cond_t cond1_1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

DataTableGeneratedKeysEntry *Keys_Table;
struct hashmap *MIB;
struct list_node *linked_list_head;

//Assisting global variables
int total_number_of_resets = 1; //Going to be usesfull when generating keys, we start at 1 because technically creating it is updating it 1 time
struct timeval Z_ResetTime_Comparator;
time_t signal_time_assistant;
//

//Defining the variables that will hold the MIB values 
struct timeval Z_reset_times; //Basically OID 1.1.1 and 1.1.2 from the MIB
unsigned char **Z;
int systemKeySize = 0;
int systemIntervalUpdate = 0;
int systemMaxNumberOfKeys = 0;
int systemKeysTimeToLive = 0;
char configMasterKey[50];
int configFirstCharOfKeysAlphabet = 0;
int configCardinalityOfKeysAlphabet = 256;
int dataNumberOfValidKeys = 0;
int dataTableGeneratedKeys = 0;
int total_amount_of_keys_generated = 0;

//Precisamos de usar mutex e condições para evitar que os processos bloquem o programa em certos momentos, um momento critíco em que se deve evitar bloquear o programa é na abertura da socket

//int socket_initialized = 0;

sigset_t signal_set;

int KeyId_Gen(){

  srand(time(NULL)/2); //We dive the time because if we don't we will have the same ID as the requester due to the srand() beind a pseudo-random generator
  int randomNumber = rand() % 999999999 + 100000000;

  return randomNumber;
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
    memmove(temp_array, temp_Return + size - rotation_number, rotation_number);
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number);
    memmove(temp_Return, temp_array, size);
    return temp_Return;
}

unsigned char xor (char Za, char Zb, unsigned char Zc, unsigned char Zd) {
    unsigned char value_to_return = (Za ^ Zb ^ Zc ^ Zd);
    return value_to_return;
}
void xor_and_transpose_keygen(int row, int collumn, int pos_assistant){
    for(int char_counter = 0; char_counter<systemMaxNumberOfKeys;char_counter++){
        Keys_Table[pos_assistant].keyValue[char_counter] = Z[row][char_counter]^Z[char_counter][collumn];
    }
}

int random_char(char seed, int inc, int max) {
    // Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    // E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    int Pos_random = (rand() % (max-0+1)+0);
    return Pos_random;
}

int random_charZ(unsigned char seed, int inc, int max) {
    //struct timeval tv;
    //gettimeofday(&tv,NULL);
    //unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    // Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    // E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    //int row  = (rand() % (systemMaxNumberOfKeys-0+1)+0);
    int Pos_random = (rand() % (max-0+1)+0);
    return Pos_random;
}

unsigned char *rotateZ(unsigned char *M, int rotation_number) {
    size_t size = sizeof(M) / sizeof(M[0]);  // Number of bytes / Numero de caracteres
    size = systemKeySize;
    unsigned char *temp_Return = (unsigned char *)malloc(size*sizeof(unsigned char));
    //temp_Return[systemKeySize] = '\0';
    memmove(temp_Return, M, size);
    unsigned char temp_array[size];
    //temp_array[size] = '\0';

    memmove(temp_array, temp_Return + size - rotation_number, rotation_number);
    memmove(temp_array + rotation_number, temp_Return, size - rotation_number);
    memmove(temp_Return, temp_array, size);

    return temp_Return;
}

void new_list_node(struct list_node **current_node, struct Mib_Entry node_info){
    struct list_node *new_node;
    new_node = (struct list_node*)malloc(sizeof(struct list_node));
    new_node->next = NULL;
    new_node->prev = *current_node;
    new_node->Mib_entry = node_info;
    memcpy(new_node->oid,node_info.oid,strlen(node_info.oid));
    (*current_node)->next = new_node;
    *current_node = new_node;
}

void list_tester(struct list_node **current_node){
    while((*current_node)->next != NULL){
        printf("List_node->%s\n",(*current_node)->oid);
        printf("List_ObjectTYPE->%s\n",(*current_node)->Mib_entry.Object_Type);
        *current_node = (*current_node)->next;
    }
    if((*current_node)->next == NULL){
        printf("List_node->%s\n",(*current_node)->oid);
        printf("List_ObjectTYPE->%s\n",(*current_node)->Mib_entry.Object_Type);
    }
    
    *current_node = linked_list_head; 
}


void MibSetter(int fd, struct list_node **head){ //We could use a double linked list or even a simple linked list to do this instead of a dependency based hashmap
    //This function returns a doubley linked list to act as a way to store the MIB values, this is automatic and should be able to automatically store a MIB file
    //As a linked list. We didn't consider the key table in this since we will hard code the parameters for when the client asks to acess the key table
    //This has a lot of hardcoded strings, it would be better to do some optimizing or use some regex maybe. The easiest way would probably be to use the line number and to just tell 
    //It to ignore the description part and to stop registering or avoid a set number of lines when encountering the Table name or to just scrap the MibEntry if it acknolodges that it is part of the table description
    char buffer[256];
    char small_buffer[2];
    char *found;
    struct list_node *temp_head = *head;
    struct Mib_Entry temp_entry;
    char interest_string1[] = "OBJECT-TYPE";
    char interest_string2[] = "SYNTAX ";
    char interest_string3[] = "MAX-ACCESS ";
    char interest_string4[] = "STATUS ";
    char interest_string5[] = "DESCRIPTION ";
    char interest_string6[] = "::=";
    char avoid_string[] = "snmpKeysMib";
    char avoid_string2[] = "INDEX {";
    char avoid_string3[] = "DataTableGeneratedKeysEntryType ::=";
    char stop_inserting_string[] = "SEQUENCE {";
    int char_counter = 0;
    int lock = 0;
    int new_node_added = 0;
    while(read(fd,small_buffer,1) > 0){
        
        /*if(char_counter == 255){
            memset(buffer,'\0',256);
            char_counter = 0;
        }*/
        strcat(buffer,small_buffer);
        if(small_buffer[0] == '\n'){
            //printf("buffer->%s\n",buffer);
            /*if((found = strstr(buffer,stop_inserting_string)) != NULL && lock == 0){
                return -1;
            }*/
            if((found = strstr(buffer,interest_string1)) != NULL && lock == 0){
                //printf("found->%s|buffer->%s",found,buffer);
                memmove(temp_entry.Object_Type,buffer,(strlen(buffer)-strlen(found)-1));
                //printf("IN THE NODE->%s\n",temp_entry.Object_Type);
                lock = 1;
            }
            if((found = strstr(buffer,interest_string2)) != NULL && lock == 0){
                memmove(temp_entry.Syntax,buffer+strlen(interest_string2),(strlen(buffer)-strlen(interest_string2)));
                //memset(buffer,'\0',256);
                lock = 1;
            }
            if((found = strstr(buffer,interest_string3)) != NULL && lock == 0){
                if(strstr(buffer,"read-only") != NULL){
                    temp_entry.max_access = 1;
                }
                if(strstr(buffer,"read-write") != NULL){
                    temp_entry.max_access = 2;
                }
                if(strstr(buffer,"not-accessible") != NULL){
                    temp_entry.max_access = 3;
                }
                //memset(buffer,'\0',256);
                lock = 1;
            }
            if((found = strstr(buffer,interest_string4)) != NULL && lock == 0){
                if(strstr(buffer,"current") != NULL){
                    temp_entry.Status = 1;
                }
                if(strstr(buffer,"mandatory") != NULL){
                    temp_entry.Status = 2;
                }
                //memset(buffer,'\0',256);
                lock = 1;
            }
            if((found = strstr(buffer,interest_string5)) != NULL && lock == 0){
                //For praticality we ignore the description since we don't really get any benefit with storing it, simplifying the mib hashmap creation
            }
            if((found = strstr(buffer,interest_string6)) != NULL && strstr(buffer,avoid_string) == NULL && strstr(buffer,avoid_string2) == NULL && strstr(buffer,avoid_string3) == NULL && lock == 0){
                char oid_temp = 0;
                char *output;
                int number_of_ints = 0;
                int start_point = 0;
                if(strstr(buffer,"{ system") != NULL){
                    oid_temp = '1';
                }
                if(strstr(buffer,"{ config") != NULL){
                    oid_temp = '2';
                }
                if(strstr(buffer,"{ data") != NULL){
                    oid_temp = '3';
                }
                if(strstr(buffer,"{ dataTableGeneratedKeysEntry") != NULL){
                    oid_temp = '4';
                }
                for(int i = 0; i < strlen(buffer);i++){
                    if(isdigit(buffer[i]) != 0){
                        number_of_ints++;
                        if(start_point == 0){
                            start_point = i;
                        }
                    }
                }
                if(number_of_ints > 0){
                    output = (char *)malloc(sizeof(char)*number_of_ints);
                    memmove(output,buffer+start_point,number_of_ints);
                    sprintf(temp_entry.oid,"1.%c.%s",oid_temp,output);
                }
                lock = 1;
                new_node_added = 1;
                new_list_node(&temp_head,temp_entry);
            }
            if(strstr(buffer,avoid_string3) != NULL){
                new_node_added = 1;
            }
        char_counter == 0;
        lock = 0;
        memset(buffer,'\0',256);
        //hashmap_set(MIB, &(struct Mib_Entry){.Object_Type = temp_entry.Object_Type,.max_access = temp_entry.max_access,.oid=temp_entry.oid,.Status=temp_entry.Status});
        }
        char_counter++;
        if(new_node_added == 1){
            new_node_added = 0;
            memset(temp_entry.oid,'\0',20);
            memset(temp_entry.Object_Type,'\0',50);
            memset(temp_entry.Syntax,'\0',20);
        }
    }   
    //list_tester(&temphead_testing);
}

//Assist function for the message_processor, it decluters it by removing the search for the list node from it.
//We should end up populating the Request Storage this way
//We return 1 if there is a error getting the node, this is basicly only be usefull if the client request an invalid OID
void get_requested_nodes(char *requested_node, struct list_node **List_Head, struct Request_Storage *Storage){
    struct list_node *node = *List_Head;
    size_t request_cluster_size = strlen(requested_node);
    char resized_request_buffer[request_cluster_size];
    memmove(resized_request_buffer,requested_node+1, request_cluster_size-2); //We remove the first '(' and last ')' from the requested_node
    resized_request_buffer[request_cluster_size-2] = '\0';
    char *substring_comma;
    char request_value_substr[SET_REQUEST_MAX_ASNWER_SPACE];
    char request_value_oid[10];
    int requested_node_found = 0;
    int requests_stored = 0;
    int chars_to_comma = 0;
    int error = 0;
    substring_comma = strtok(resized_request_buffer,";");

    while(substring_comma != NULL) {
        //Request value extraction
        //printf("substring_comma->%s\n",substring_comma);
        size_t string_size = strlen(substring_comma);
        for(chars_to_comma = 0; chars_to_comma<string_size;chars_to_comma++){
            //printf("Test->%c\n",substring_comma[chars_to_comma]);
            if(substring_comma[chars_to_comma] == ','){
                chars_to_comma++;
                break;
            }
        }
        //printf("Tester1->%c\n",substring_comma[chars_to_comma]);
        memmove(request_value_substr,substring_comma+chars_to_comma,(string_size-chars_to_comma));
        memmove(request_value_oid,substring_comma,(chars_to_comma-1));
        request_value_oid[(chars_to_comma)-1] = '\0';
        printf("%s,%s\n",request_value_substr,request_value_oid);
        //--
        while(requested_node_found != 1){
            //printf("%s|%s\n",node->oid,request_value_oid);
            if(strcmp(node->oid,request_value_oid) == 0){
                requested_node_found = 1;
                if(requests_stored < Storage->number_of_requests){              
                    printf("Current_node -> %s|%d\n",node->oid,node->Mib_entry.max_access);      
                    size_t request_value_size = strlen(request_value_substr)+1;
                    memmove(Storage->Rqst_Strg[requests_stored].set_value,request_value_substr,request_value_size);
                    Storage->Mib_Entry_Storage[requests_stored] = node->Mib_entry;
                    Storage->Rqst_Strg[requests_stored].Entry_Mib = node->Mib_entry;
                    //printf("Hello->%s\n",Storage->Rqst_Strg[requests_stored].set_value);
                    //requests_stored++;
                }else{
                    //perror("Error: Attempted to store more requests than what was originally requested");
                    
                    break;
                }
                node = *List_Head; //Just to simplify the search process
            }else{
                node = node->next;
            }
            if(node==NULL){
                printf("End of the MIB reached, no oid found\n");
                Storage->Rqst_Strg[requests_stored].Oid_error = 1;
                break;
            }
            //printf("T->%s\n",node->oid);
        }
        requests_stored++;
        memset(request_value_substr,'\0',SET_REQUEST_MAX_ASNWER_SPACE);
        memset(request_value_oid,'\0',10);
        substring_comma = strtok(NULL,";");
        requested_node_found = 0;
        node = *List_Head;
    }
    free(substring_comma);
}

//We use this to assist in processing the message, it should be able to process most of the requests
//The objetive of this function is to retrieve the MIB assets the client wants to access from the message and how many.
//Important positions 4th and () enclosed stuff
int message_processor(char *message_buffer, struct Request_Storage *Storage, struct list_node **list_head){
    
    char buffer[1000];
    char ID_store[10];
    int request_type = 0; //1-> Get, 2->Set 0->error (usefull for checking OID)
    int end_of_message = 0;
    int PDU_container_counter = 0;
    char *substring_token;
    size_t full_message_size = strlen(message_buffer);
    message_buffer[full_message_size+1] = '\0';
    substring_token = strtok(message_buffer, "|");
    
    while(end_of_message != 1){
        while(substring_token != NULL){
            if(PDU_container_counter == 3){
                Storage->Manager_ID = (char *)malloc(sizeof(char)*strlen(substring_token)+2);
                sprintf(Storage->Manager_ID,"%s",substring_token);
                Storage->Manager_ID[strlen(substring_token)+1] = '\0';
            }
            if(PDU_container_counter == 4){
                strcat(ID_store,substring_token);
            }
            if(PDU_container_counter == 5){
                request_type = atoi(substring_token);
                if(request_type<1 && request_type >2){
                    request_type = 0;
                    perror("Invalid type of operation detected in request!");
                }
            }
            if(PDU_container_counter == 6){
                Storage->number_of_requests = atoi(substring_token);
                Storage->Mib_Entry_Storage = (struct Mib_Entry*)malloc(sizeof(struct Mib_Entry)*(Storage->number_of_requests));
                Storage->Rqst_Strg = (struct Request*)malloc(sizeof(struct Request)*(Storage->number_of_requests));
                strcat(Storage->Requester_ID,ID_store);
                for(int request_set_counter = 0; request_set_counter<Storage->number_of_requests;request_set_counter++){
                    Storage->Rqst_Strg[request_set_counter].set_value = (char*)malloc(sizeof(char)*SET_REQUEST_MAX_ASNWER_SPACE);
                }
            }
            if(PDU_container_counter == 7){
                get_requested_nodes(substring_token,list_head,Storage);
                return request_type;
            }
            PDU_container_counter++;
            substring_token = strtok(NULL, "|");
        }
        end_of_message = 1;
    }
    free(substring_token);
    return request_type;
}

void generate_key(int Keys_Table_Position){
    printf("Number of updates in Z->%d\n",total_number_of_resets);
    //Formula for the key generation
    //int ltime = time(NULL);
    srand(total_number_of_resets + Z[0][0]);
    int row  = (rand() % (systemMaxNumberOfKeys-0+1)+0);
    srand(Z[row][0]);
    int collumn = (rand() % (systemMaxNumberOfKeys-0+1)+0);
    //unsigned char *new_key = xor_and_transpose_keygen(row,collumn);
    Keys_Table[Keys_Table_Position].keyValue = (unsigned char *)malloc(sizeof(unsigned char)*systemKeySize+1);
    xor_and_transpose_keygen(row,collumn,Keys_Table_Position);
    //memmove(Keys_Table[Keys_Table_Position].keyValue,assistant,systemKeySize);
    //Populate the Keys_Table in the correct position
    Keys_Table[Keys_Table_Position].keyExpirationDate = time(NULL) + systemKeysTimeToLive;
    Keys_Table[Keys_Table_Position].keyExpirationTime = time(NULL) + systemKeysTimeToLive;
    Keys_Table[Keys_Table_Position].keyId = KeyId_Gen();
    //memmove(Keys_Table[Keys_Table_Position].keyValue,new_key,systemKeySize);

    //Signal the condition
    pthread_cond_signal(&cond1);
    //Reset the update timer
    gettimeofday(&Z_ResetTime_Comparator,NULL);
}

DataTableGeneratedKeysEntry Key_List_Node(char *Requester_ID){
    for(int key_table_searcher = 0; key_table_searcher<dataNumberOfValidKeys; key_table_searcher++){
        if(strcmp(Requester_ID,Keys_Table[key_table_searcher].keyRequester) == 0){
            return Keys_Table[key_table_searcher];
        }
    }
}

//This function is responsible for the checking if the request can be fulfilled (checking access, etc ...) and if it is, to populate the answer object
//Returns values 1->Get Answer, 2-> Set Answer, 10->Get Error, 20->Set Error
void request_verifier(struct Answer *answer_ptr, struct Mib_Entry Entry, struct list_node **list, int operation_type, char* request_value, char *Manager_ID){
    struct list_node *function_list = list;
    struct tm *time_Helper;
    answer_ptr->answer_value[511] = '\0';
    for(int list_node_num = 0; function_list!=NULL; list_node_num++){
        if(strcmp(function_list->oid,Entry.oid) == 0){
            answer_ptr->oid_accessed[19] = '\0';
            sprintf(answer_ptr->oid_accessed,"%s",function_list->oid);
            
            //printf("Max_access->%d\n",Entry.max_access);
            switch(Entry.max_access){
                case 1: //read-only

                    if(operation_type == 2){
                        answer_ptr->answer_type = (operation_type*10);
                        strcat(answer_ptr->answer_value,"1\0");
                    }else{
                        answer_ptr->answer_type = 1;
                        
                        DataTableGeneratedKeysEntry key_node_to_return = Key_List_Node(Manager_ID);
                        if(strcmp(Entry.oid,"1.1.1") == 0){
                            char date_String[100];
                            time_Helper = localtime(&Z_reset_times);
                            strftime(date_String, sizeof(date_String), "%Y-%m-%d",time_Helper);
                            sprintf(answer_ptr->answer_value,"%s",date_String);
                        }
                        if(strcmp(Entry.oid,"1.1.2") == 0){
                            char date_String[100];
                            time_Helper = localtime(&Z_reset_times);
                            strftime(date_String, sizeof(date_String), "%H:%M:%S",time_Helper);
                            sprintf(answer_ptr->answer_value,"%s",date_String);
                        }
                        if(strcmp(Entry.oid,"1.3.1") == 0){
                            sprintf(answer_ptr->answer_value,"%s",configMasterKey);
                        }
                        if(strcmp(Entry.oid,"1.4.2") == 0){
                            sprintf(answer_ptr->answer_value,"%s",key_node_to_return.keyValue);
                        }
                        if(strcmp(Entry.oid,"1.4.3") == 0){
                            sprintf(answer_ptr->answer_value,"%s",key_node_to_return.keyRequester);
                        }
                        if(strcmp(Entry.oid,"1.4.4") == 0){
                            sprintf(answer_ptr->answer_value,"%d",key_node_to_return.keyExpirationDate);
                        }
                        if(strcmp(Entry.oid,"1.4.5") == 0){
                            sprintf(answer_ptr->answer_value,"%d",key_node_to_return.keyExpirationTime);
                        }
                    }
                    break;
                case 2: //read-write
                    if(operation_type == 1){
                        
                        answer_ptr->answer_type = operation_type;
                        
                        if(strcmp(Entry.oid,"1.1.3") == 0){
                            sprintf(answer_ptr->answer_value,"%d",systemKeySize);
                        }
                        if(strcmp(Entry.oid,"1.1.4") == 0){
                            sprintf(answer_ptr->answer_value,"%d",systemIntervalUpdate);
                        }
                        if(strcmp(Entry.oid,"1.1.5") == 0){
                            sprintf(answer_ptr->answer_value,"%d",systemMaxNumberOfKeys);
                        }
                        if(strcmp(Entry.oid,"1.1.6") == 0){

                            sprintf(answer_ptr->answer_value,"%d",systemKeysTimeToLive);
                        }
                        if(strcmp(Entry.oid,"1.2.1") == 0){
                            sprintf(answer_ptr->answer_value,"%s",configMasterKey);
                        }
                        if(strcmp(Entry.oid,"1.2.2") == 0){

                            sprintf(answer_ptr->answer_value,"%d",configFirstCharOfKeysAlphabet);
                        }
                        if(strcmp(Entry.oid,"1.2.3") == 0){
                            sprintf(answer_ptr->answer_value,"%d",configCardinalityOfKeysAlphabet);
                        }
                        if(strcmp(Entry.oid,"1.4.6") == 0){
                            int key_was_found = 0;
                            //We know the ID size so we can just make it so that the first digits of the request value are the ID and the last digit is the visivility option
                            for(int key_table_searcher = 0; key_table_searcher<systemMaxNumberOfKeys && key_was_found != 1; key_table_searcher++){
                                //printf("atoi->%d key_table->%d\n",atoi(request_value),Keys_Table[key_table_searcher].keyId);
                                if(atoi(request_value) == Keys_Table[key_table_searcher].keyId){
                                    int helper;
                                    char intermidiary_helper[5];
                                    intermidiary_helper[4] = '\0';
                                    char helper_char[50];
                                    //helper_char[49] = '\0';
                                    int offset = 0;
                                    for(int convert_int_num = 0; convert_int_num<systemKeySize;convert_int_num++){
                                        helper = Keys_Table[key_table_searcher].keyValue[convert_int_num];
                                        sprintf(intermidiary_helper,"%d*",helper);
                                        size_t intermidiary_helper_size = strlen(intermidiary_helper);
                                        memmove(helper_char+offset,intermidiary_helper,intermidiary_helper_size);
                                        offset = offset+intermidiary_helper_size;
                                    }
                                    
                                    if(strcmp(Manager_ID,Keys_Table[key_table_searcher].keyRequester) == 0){
                                        if(Keys_Table[key_table_searcher].keyVisibility == 0){
                                            //No visibility for anyone
                                            answer_ptr->answer_type = 10;
                                            strcat(answer_ptr->answer_value,"6\0");
                                        }
                                        //printf("%s,%s\n",Keys_Table[key_table_searcher].keyRequester,Requester_ID);
                                        if(Keys_Table[key_table_searcher].keyVisibility == 1 && strcmp(Manager_ID,Keys_Table[key_table_searcher].keyRequester) == 0){
                                            //Visibility only for the Key Original Requester
                                            //memmove((char *)answer_ptr->answer_value,Keys_Table[key_table_searcher].keyValue,systemKeySize);
                                            answer_ptr->answer_type = 1;
                                            sprintf(answer_ptr->answer_value,"%d*%s",Keys_Table[key_table_searcher].keyId,helper_char);
                                            //answer_ptr->answer_value[strlen(helper_char)+1] = '\0';
                                        }
                                        if(Keys_Table[key_table_searcher].keyVisibility == 1 && strcmp(Manager_ID,Keys_Table[key_table_searcher].keyRequester) != 0){
                                            //You aren't the original request so no visibility
                                            printf("Tagala\n");
                                            answer_ptr->answer_type = 10;
                                            strcat(answer_ptr->answer_value,"6\0");
                                        }
                                        key_was_found = 1;
                                    }
                                    if(Keys_Table[key_table_searcher].keyVisibility == 2){
                                        //Every one gets to see you!
                                        //memmove((char *)answer_ptr->answer_value,Keys_Table[key_table_searcher].keyValue,systemKeySize);
                                        answer_ptr->answer_type = 1;
                                        sprintf(answer_ptr->answer_value,"%d*%s",Keys_Table[key_table_searcher].keyId,helper_char);
                                        //answer_ptr->answer_value[strlen(helper_char)+1] = '\0';
                                        key_was_found = 1;
                                    }
                                    break;
                                }else{
                                    //We haven't found a key with a similar keyID
                                    if(answer_ptr->answer_type != 10){
                                        answer_ptr->answer_type = 10;
                                        strcat(answer_ptr->answer_value,"9\0");
                                    }
                                }
                            }
                        }
                    }
                    if(operation_type == 2){
                        //Since altering most of the variables woul be quite chaotic for the agent we just alter the keyVisibility, otherwise we just tell the client he doesn't have the right permission
                        answer_ptr->answer_type = operation_type;
                        int visibility_error = 0;
                        if(strcmp(Entry.oid,"1.4.6") == 0){
                            printf("New key request\n");
                            int key_visibility = atoi(request_value);
                            if(key_visibility<0 && key_visibility>2){
                                //printf("Invalid keyVisibility value");
                                answer_ptr->answer_type = 20;
                                strcat(answer_ptr->answer_value,"5\0");
                                visibility_error = 1;
                            }
                            int key_was_found = 0;
                            int empty_position_for_key = -1; //This can be true for 2 cases, a space is occupied by no keys or we found a space with an expired key, we will use this opportunity to check all keys for experired ones
                            //We know the ID size so we can just make it so that the first digits of the request value are the ID and the last digit is the visivility option
                            for(int key_table_searcher = 0; key_table_searcher<systemMaxNumberOfKeys && visibility_error != 1; key_table_searcher++){
                                if(Keys_Table[key_table_searcher].keyRequester != NULL){
                                    if(strcmp(Manager_ID,Keys_Table[key_table_searcher].keyRequester) == 0){
                                        //We already have a key for the same client, we update with a new one
                                        empty_position_for_key = key_table_searcher;
                                        dataNumberOfValidKeys--; //We do this to keep a the true number of valid keys as a true demonstration of the num of val keys
                                    }else{
                                        //If we have valid keys but we don't have any key for the requester we need to check the spaces and if there is any dead keys
                                        int now_epoch_time = time(NULL);
                                        char current_time_char[100], keyTime_char[100];
                                        int time_diff = now_epoch_time-Keys_Table[key_table_searcher].keyExpirationDate;
                                        if(Keys_Table[key_table_searcher].keyRequester[0] == NULL){
                                            //We have an empty position
                                            empty_position_for_key = key_table_searcher;
                                        }
                                        if(time_diff >= 0 && time_diff <= systemKeysTimeToLive){
                                            //We still have a valid key in here
                                        }else{
                                            //We can delete this key and assume it's position as a valid empty position
                                            empty_position_for_key = key_table_searcher;
                                            Keys_Table[key_table_searcher].keyExpirationDate = 0;
                                            Keys_Table[key_table_searcher].keyExpirationTime = 0;
                                            Keys_Table[key_table_searcher].keyId = 0;
                                            memset(Keys_Table[key_table_searcher].keyRequester,'\0',(strlen(Keys_Table[key_table_searcher].keyRequester)+1));
                                            memset(Keys_Table[key_table_searcher].keyValue,'\0',(strlen(Keys_Table[key_table_searcher].keyValue)+1));
                                            Keys_Table[key_table_searcher].keyVisibility = 0;
                                            dataNumberOfValidKeys--;
                                            memmove((char *)answer_ptr->answer_value+9,Keys_Table[empty_position_for_key].keyValue,systemKeySize);
                                        }
                                        }
                                }else{
                                    empty_position_for_key = key_table_searcher;
                                }
                            }
                            if(empty_position_for_key != -1){
                                pthread_mutex_lock(&mutex3); //We want to avoid a race condition to write in the keys_table
                                pthread_mutex_lock(&mutex);
                                generate_key(empty_position_for_key);
                                pthread_mutex_unlock(&mutex);
                                Keys_Table[empty_position_for_key].keyRequester = (unsigned char*)malloc(sizeof(unsigned char) * (strlen(Manager_ID)+1));
                                memmove(Keys_Table[empty_position_for_key].keyRequester,Manager_ID,strlen(Manager_ID));
                                Keys_Table[empty_position_for_key].keyVisibility = key_visibility;
                                pthread_mutex_unlock(&mutex3);
                                if(Keys_Table[empty_position_for_key].keyId != 0){
                                    //Key was successfully created
                                    int helper;
                                    char intermidiary_helper[5];
                                    intermidiary_helper[4] = '\0';
                                    char helper_char[50];
                                    //helper_char[49] = '\0';
                                    int offset = 0;
                                    for(int convert_int_num = 0; convert_int_num<systemKeySize;convert_int_num++){
                                        helper = Keys_Table[empty_position_for_key].keyValue[convert_int_num];
                                        sprintf(intermidiary_helper,"%d*",helper);
                                        size_t intermidiary_helper_size = strlen(intermidiary_helper);
                                        memmove(helper_char+offset,intermidiary_helper,intermidiary_helper_size);
                                        offset = offset+intermidiary_helper_size;
                                    }
                                    sprintf(answer_ptr->answer_value,"%d*%s",Keys_Table[empty_position_for_key].keyId,helper_char);
                                    //answer_ptr->answer_value[strlen(helper_char)+1] = '\0';
                                    dataNumberOfValidKeys++;
                                }else{
                                    //Error while creating the key
                                    answer_ptr->answer_type = 20;
                                    strcat(answer_ptr->answer_value,"7\0");
                                    }
                            }else{
                                //No free spaces for a new key
                                answer_ptr->answer_type = 20;
                                strcat(answer_ptr->answer_value,"7\0");
                            }
                        }else{
                            answer_ptr->answer_type = 20;
                            strcat(answer_ptr->answer_value,"3\0");
                        }
                    }
                    break;
                case 3: //non-accesible
                    answer_ptr->answer_type = (operation_type*10);
                    strcat(answer_ptr->answer_value,"0\0");
                    break;
                default:
                    //Invalid or non expected max-access valued we shouldn't really be getting in here
                    break;
            }
            break; //Since we are done processing one of the requests we get out of the loop so as to quickly go to the remaning requests on the protocolagent if there is any 
        }
        if(function_list->next == NULL){
            break;
        }
        function_list = function_list->next;
    }
}

char *response_maker(int number_of_requests, char *buffer_ptr, struct Request_Storage Rq_Storage,struct Answer_Storage Answ_Storage, int request_type){
    char *buffer;
    buffer = (char*)malloc(sizeof(char)*1024); //We initially utilize a 512 byte buffer for the response however this can be inscreased if it is needed;
    char error_list_buffer[512];
    error_list_buffer[511] = '\0';
    memset(error_list_buffer,'\0',512);
    char good_answer_buffer[512];
    good_answer_buffer[511] = '\0';
    int treated_requests = 0;
    int number_of_errors = 0;
    int number_of_answers = 0;
    for(treated_requests = 0; treated_requests<number_of_requests;treated_requests++){
        if(Answ_Storage.Answ_Str[treated_requests].answer_type == 1 || Answ_Storage.Answ_Str[treated_requests].answer_type == 2){
            if(Answ_Storage.Answ_Str[treated_requests].answer_value[0] != '8'){
                strcat(good_answer_buffer,Answ_Storage.Answ_Str[treated_requests].oid_accessed);
            }else{
                strcat(error_list_buffer,"0");
            }
            strcat(good_answer_buffer,",");
            strcat(good_answer_buffer,Answ_Storage.Answ_Str[treated_requests].answer_value);
            strcat(good_answer_buffer,";");
            number_of_answers++;
        }else{
            if(Answ_Storage.Answ_Str[treated_requests].answer_value[0] != '8' || Rq_Storage.Rqst_Strg[treated_requests].Oid_error != 1){
                strcat(error_list_buffer,Answ_Storage.Answ_Str[treated_requests].oid_accessed);
            }else{
                strcat(error_list_buffer,"0");
            }
            strcat(error_list_buffer,",");
            strcat(error_list_buffer,Answ_Storage.Answ_Str[treated_requests].answer_value);
            strcat(error_list_buffer,";");
            number_of_errors++;
        }
    }
    if(number_of_errors == 0){
        number_of_errors = 1;
        sprintf(error_list_buffer,"0,0;",NULL);
    }
    if(number_of_answers == 0){
        number_of_answers = 1;
        sprintf(good_answer_buffer,"0,0;",NULL);
    }
    good_answer_buffer[strlen(good_answer_buffer)] = '\0';
    error_list_buffer[strlen(error_list_buffer)] = '\0';
    sprintf(buffer_ptr,"0|0|0|%s|%d|%d|(%s)|%d|(%s)|",Rq_Storage.Requester_ID,request_type,number_of_requests,good_answer_buffer,number_of_errors,error_list_buffer);
    return buffer;
}

// Definição das funções para a threads.

// No server usamos a solução sugerida pelo professor que é o uso de um ficheiro com o número de pedidos total
//Só precisamos de um arguemnto a ser recebido, que é o buffer associado ao pedido, estes depois faz tudo desde tratamento dos dados a
//processamento.
//1 thread por pedido. Enviamos o resultado para a CommAgent ou fazemos aqui o envio por uma questão de simplicidade/efc?
void *ProtocolAgent(void *arg){
    
    dados_ProtocolAgent *args = (dados_ProtocolAgent *)arg;
    struct sockaddr_in client_address = args->client_addrss;
    int new_socket_fd = args->temp;
    char *Client_Request = args->Request_Message;
    char Client_request_to_print[strlen(Client_Request)+1];
    Client_request_to_print[strlen(Client_Request)] = '\0';
    memmove(Client_request_to_print,Client_Request,strlen(Client_Request));
    socklen_t address_size = sizeof(client_address);
    struct Request_Storage Rqst_Storage;
    struct Request_Storage *Strg_ptr;
    Strg_ptr = &Rqst_Storage;
    struct list_node *temp_node = linked_list_head;
    int request_type = message_processor(Client_Request,Strg_ptr,&temp_node);
    struct  Answer_Storage Answer_Str;
    Answer_Str.Answ_Str = (struct Answer*)malloc(sizeof(struct Answer)*Rqst_Storage.number_of_requests+1);

    printf("Current request type->%d, number %s, from requester %s \nContent: %s",request_type,Rqst_Storage.Requester_ID,Rqst_Storage.Manager_ID,Client_request_to_print);

    if(request_type == 0){
        //pthread_exit(0); 
    }
    
    for(int request_number = 0; request_number < Rqst_Storage.number_of_requests; request_number++){
        struct Answer *new_answer_ptr;
        new_answer_ptr = &Answer_Str.Answ_Str[request_number];
        (*new_answer_ptr).position_RequestStorage = request_number;
        printf("\n");
        if(request_type == 1){
            printf("Get Request\n");
        }
        if(request_type == 2){
            printf("Set request\n");
        }
        if(request_type == 0){
            printf("Invalid request\n");
            Answer_Str.Answ_Str[request_number].answer_type == request_type*10;
            strcat(Answer_Str.Answ_Str[request_number].answer_value,"2");
        }
        
        if(Rqst_Storage.Rqst_Strg[request_number].Oid_error != 1){
            if(request_type == 1 || request_type == 2){
                request_verifier(new_answer_ptr,Rqst_Storage.Rqst_Strg[request_number].Entry_Mib,temp_node,request_type,Rqst_Storage.Rqst_Strg[request_number].set_value,Rqst_Storage.Manager_ID);
            }
        }else{
            Answer_Str.Answ_Str[request_number].answer_type = request_type*10;
            strcat(Answer_Str.Answ_Str[request_number].answer_value,"8");
        }
    }
    char answer_buffer[1025];
    answer_buffer[1024] = '\0';
    response_maker(Rqst_Storage.number_of_requests,answer_buffer,Rqst_Storage,Answer_Str,request_type);
    printf("Request answer %s\n",answer_buffer);
    if(sendto(new_socket_fd,answer_buffer,1024,0,(struct sockaddr *)&client_address,sizeof(client_address))<0){
        printf("Couldn't send response");
    }
    
    pthread_exit(NULL);
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
    struct sockaddr_in server, client;  // Esta socket é mais por questões de facilitar a vida ao usar o inet
    socklen_t server_len = sizeof(server);
    socklen_t client_len = sizeof(client);

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
    
    while(1){
        
        char buffer[1024];
        size_t num_bytes_waiting = recvfrom(Udp_Server_Socket_fd,buffer,512,0,(struct sockaddr *)&client,&client_len); //We store the client socket info so as to send it to the protocol module with a dedicated thread to avoid wasting time on the main process

        if (num_bytes_waiting > 0){
                buffer[strlen(buffer)+1]= '\0';
                dados_ProtocolAgent PrtAgent_Args;
                PrtAgent_Args.client_addrss = client;
                PrtAgent_Args.Request_Message = buffer;
                PrtAgent_Args.temp = Udp_Server_Socket_fd;
                pthread_t ProtocolAgent_Thread;
                int thread_sucess = pthread_create(&ProtocolAgent_Thread,NULL,ProtocolAgent,&PrtAgent_Args);
                if(thread_sucess != 0){
                    perror("Can't answer the request (Protocol Thread Failed)");
                }

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
    int contador_row = 0;
    int contador_collumn = 0;
    
    // Geração das matrizes e inicialização do keyagent
    unsigned char Za[K_T][K_T];
    unsigned char Zb[K_T][K_T]; 
    unsigned char Zc[K_T][K_T];
    unsigned char Zd[K_T][K_T];
    //unsigned char Z[K_T][K_T];
    Z = (unsigned char **)malloc(sizeof(unsigned char *)*K_T);
    for(int counter = 0;counter!=K_T;counter++){
        Z[counter] = (unsigned char *)malloc(sizeof(unsigned char *)*K_T);
    }

    for (contador_row = 0; contador_row != K_T; contador_row++) {
        for (contador_collumn = 0; contador_collumn != K_T; contador_collumn++) {
            // printf("Teste_NUm:%d",contador_collumn);
            Za[contador_row][contador_collumn] = '\0';
            Zb[contador_row][contador_collumn] = '\0';
            Zc[contador_row][contador_collumn] = '\0';
            Zd[contador_row][contador_collumn] = '\0';
            Z[contador_row][contador_collumn] = '\0';
        }
    }

    unsigned char *Chave_Mestra = Gen_Chave_Mestra(2 * K_T);  // Continuamos a devolver como array ou como int?
    memmove(configMasterKey,Chave_Mestra,K_T*2);

    unsigned char Chave_M1[K_T];
    unsigned char Chave_M2[K_T];
    Chave_M1[K_T] = '\0';
    Chave_M2[K_T] = '\0';

    memcpy(Chave_M1, Chave_Mestra, K_T);
    memcpy(Chave_M2, Chave_Mestra + K_T, K_T);

    // Preenchimento das Matrizes Za & Zb
    

    // Za -> Filled per Row
    for (contador_row = 0; contador_row < K; contador_row++) {
        char temp_m[K_T];
        temp_m[K_T] = '\0';
        memmove(temp_m, Chave_M1, sizeof(temp_m));
        char *rotated_array = rotate(temp_m, contador_row);
        for (contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            if (contador_row == 0) {
                Za[contador_row][contador_collumn] = Chave_M1[contador_collumn];
            } else {
                Za[contador_row][contador_collumn] = rotated_array[contador_collumn];
            }
        }
        printf("%s\n", Za[contador_row]);
    }

    // Zb -> Filled per Collumn
    for (contador_collumn = 0; contador_collumn < K; contador_collumn++) {
        char temp_m2[K_T];
        temp_m2[K_T] = '\0';
        memmove(temp_m2, Chave_M2, sizeof(temp_m2));
        char *rotated_array2 = rotate(temp_m2, contador_collumn);
        for (contador_row = 0; contador_row < K; contador_row++) {
            if (contador_collumn == 0) {
                Zb[contador_row][contador_collumn] = Chave_M2[contador_row];
            } else {
                Zb[contador_row][contador_collumn] = rotated_array2[contador_row];
            }
        }
    }

    // Zc & Zd -> Filled by random char generated trough the random() function, this random char is calculated from a random number with the original Za or Zb character
    // Of the current position in the matrix as a seed.
    for (contador_row = 0; contador_row < K; contador_row++) {
        for (contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            Zc[contador_row][contador_collumn] = random_char(Za[contador_row][contador_collumn], Min, Max);  // Estamos a ficar com chars a mais no array
            Zd[contador_row][contador_collumn] = random_char(Zb[contador_row][contador_collumn], Min, Max);
        }
    }

    // Z matrix -> Filled by xored elements from all the previous Matrixes.
    // Procurar como gerar ou perguntar ao professor se devemos tentar usar true random numbers em vez de PRNG do C. Podemos usar aplicações nativas do Unix ou Windows
    // Mas não estou a ver como podemos usar uma seed dessa maneira visto que até agora não encontrei aplicações dessas funções com uma seed
    for (contador_row = 0; contador_row < K; contador_row++) {
        for (contador_collumn = 0; contador_collumn < K; contador_collumn++) {
            unsigned char temp = xor(Za[contador_row][contador_collumn],
                                     Zb[contador_row][contador_collumn],
                                     Zc[contador_row][contador_collumn],
                                     Zd[contador_row][contador_collumn]);
            Z[contador_row][contador_collumn] = temp;
            printf("%d*",(int) Z[contador_row][contador_collumn]);
        }
        printf("\n");
    }

    int mem_block_size = K_T*K_T;
    //printf("MEM_BLOCK_SIZE => %d",mem_block_size);

    memmove(dados->Z_S,Z, mem_block_size * sizeof(unsigned char));
    
    // Obtenção do tempo
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int Time_Initilized_sec = tv.tv_sec;
    //printf("Tempo:%d\n", Time_Initilized_sec);
    //printf("K_T->%d\n",K_T);

    // -----------------
    while (1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond1, &mutex);
            printf("Z Table Updating\n");
            total_number_of_resets++;
            gettimeofday(&Z_reset_times, NULL);
            signal_time_assistant = Z_ResetTime_Comparator.tv_sec;
            // Process and update Z matrix
            // Rotate each row a random number
            for (contador_row = 0; contador_row < K_T; contador_row++) {
                int rotation_number = random_charZ(Z[contador_row][0], 0, K_T - 1);
                //printf("rotation_number: %d\n", rotation_number);
                unsigned char *temp_row = rotateZ(Z[contador_row], rotation_number);
                
                for (contador_collumn = 0; contador_collumn < K_T; contador_collumn++) {
                    Z[contador_row][contador_collumn] = temp_row[contador_collumn];
                    
                }
                /*write(0,Z[contador_row],10);
                printf("\n");*/
            }

            for (contador_collumn = 0; contador_collumn < K_T; contador_collumn++) {
                int rotation_number = random_charZ(Z[0][contador_collumn], 0, K_T - 1);
                //printf("rotation_number: %d\n", rotation_number);
                unsigned char *column = (unsigned char*)malloc(K_T * sizeof(unsigned char));
                for (int i = 0; i < K_T; i++) {
                    column[i] = Z[i][contador_collumn];
                }
                
                unsigned char *temp_row = rotateZ(column, rotation_number);
                //printf("Rotated%s\n",temp_row);
                for (contador_row = 0; contador_row < K_T; contador_row++) {
                    Z[contador_row][contador_collumn] = temp_row[contador_row];
                    //printf("%d*",(int) Z[contador_row][contador_collumn]);
                }
                //printf("\n"); NÃO TIRAR ESTE E O DE CIMA CASO O PROFESSOR PEÇA PARA VER
            }
            memmove(dados->Z_S,Z,mem_block_size * sizeof(unsigned char));
            Time_Initilized_sec = tv.tv_sec;
            printf("Z Table Updated\n");
            pthread_mutex_unlock(&mutex);
    }
}

void *timealiveagent(void *arg){
    time_t initialization_time;
    initialization_time = time(NULL);
    time_t time_alive = 0;
    time_t accumulator;
    while(1){
        if((time_alive = time(NULL)-initialization_time)==5){
            accumulator+=time_alive;
            printf("Agent has been active for %ld seconds \n",accumulator);
            printf("Currently holds %d, validKeys\n", dataNumberOfValidKeys);
            initialization_time = time(NULL);
        }
    }
}

int main(int argc, char *argv[]) {
    // Identificadores para cada Thread
    pthread_t KeyAgent_ID;
    pthread_t CommManager_ID;
    pthread_t ProtocolMod_ID;
    pthread_t TimeAlive_ID;
    //--------------------------------
    linked_list_head = (struct list_node*)malloc(sizeof(struct list_node));
    linked_list_head->next = NULL;
    linked_list_head->prev = NULL;
    struct list_node *node_to_use = linked_list_head; //We use this so as to not have to worry about knowing where the head is

    // Guardar dados do servidor para enviar como argumento
    dados_CommAgent args_commagent;

    // Tempo retirado com a inicilização do programa (Time_Since_Initiliaze)
    struct timeval Z_ResetTime_Comparator;
    gettimeofday(&Z_ResetTime_Comparator, NULL);
    int Time_Initilized_sec = Z_ResetTime_Comparator.tv_sec;             // seconds
    long int Time_Initilized_microsec = Z_ResetTime_Comparator.tv_usec;  // microseconds
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

    int mib_file_fd = 0;
    mib_file_fd = open("MIB_file.mib",O_RDONLY);
    if(mib_file_fd == -1){
        perror("Failed to open the Mib config file");
        exit(-1);
    }

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

    //KeysTable initialization
    systemMaxNumberOfKeys = K_T;
    systemIntervalUpdate = 10;
    Keys_Table = (DataTableGeneratedKeysEntry*)malloc(sizeof(DataTableGeneratedKeysEntry)*systemMaxNumberOfKeys);
    //-------------------


    // Setting up the mib as a doubley linked list
        MibSetter(mib_file_fd,&node_to_use);
        printf("T->%s\n",node_to_use->next->oid);
        list_tester(&node_to_use);
    //-------------------

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
    systemKeySize = K_T;

    unsigned char *temp_z = &args_keyagent.Z_S;

    char buffer_numpedidos[11];
    buffer_numpedidos[11] = '\0';

    pthread_create(&CommManager_ID, NULL, CommAgent, &args_commagent);
    pthread_create(&KeyAgent_ID, NULL, KeyAgent, &args_keyagent);
    pthread_create(&TimeAlive_ID,NULL, timealiveagent, NULL);
    pthread_detach(CommManager_ID);
    pthread_detach(KeyAgent_ID);

    gettimeofday(&Z_ResetTime_Comparator, NULL);
    gettimeofday(&Z_reset_times, NULL);
    signal_time_assistant = Z_ResetTime_Comparator.tv_sec;

    //sigwait(&signal_set, &signumber); //Vamos fazer com que o programa espere que a porta fique 100% operacional antes de avançar

    while(1){
        if (Z_ResetTime_Comparator.tv_sec - signal_time_assistant > systemIntervalUpdate){
            pthread_cond_signal(&cond1);
            signal_time_assistant = Z_ResetTime_Comparator.tv_sec;
        }
        gettimeofday(&Z_ResetTime_Comparator, NULL);
        //sleep(1);
    }

    return 0;
}