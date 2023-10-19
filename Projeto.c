#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define K 10 //Têm de ser retirado do ficheiro de config depois
#define Min 0 //Minimo para as funções random, têm de ser retirado do ficheiro em versões posteriores
#define Max 255 //Máximo para as funções random, o nosso objetivo é obter valores ASCII válidos, dai 255 como máximo, mesma situação do Min
//Têmos de criar uma struct para a MIB

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

unsigned char xor(unsigned char Za,unsigned char Zb,unsigned char Zc,unsigned char Zd){
    printf("Za:%c,Zb:%c,Zc:%c,Zd:%c",Za,Zb,Zc,Zd);
    unsigned char value_to_return = Za^Zb^Zc^Zd; 
    return value_to_return;
}

unsigned char random_char(char seed,int inc, int max){
    //Assumi que este era o caso uso da seed visto que não estava a ver outra maneira de isto ser feito
    //E na net este foi o exemplo mais frequente que encontrei
    srand(seed);
    unsigned char Pos_random=(char)('0' + rand() % 255);
    //printf("Pos_random gerado = %d\n",Pos_random);
    return Pos_random;
}

int main(int argc, char *argv[]) {
    
  //Temos de fazer a leitura do ficheiro para tirar as configurações, têmos também de decidir como fica o formato
    size_t K_T = K; //Ao que parece se for realizada qualquer operação sobre um valor do #define o mesmo fica alterado permanentemente
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
    /*for(int N_chaves = 0;N_chaves<2;N_chaves++){
        printf("N_chaves:%d\n",N_chaves);
        if(N_chaves == 0){
            memmove(Chave_MX[N_chaves],Chave_Mestra,K*sizeof(char)); //Ficamos com os 2 pares de chaves necessários
        }else{
            memmove(Chave_MX[N_chaves],Chave_Mestra+((N_chaves)*K),K*sizeof(char)); //Ficamos com os 2 pares de chaves necessários
        }
    }*/
    memcpy(Chave_M1,Chave_Mestra,10);
    memcpy(Chave_M2,Chave_Mestra+10,10);
    printf("K:%d\n",K);
    printf("Chave_Mestra:%s\n",Chave_Mestra);
    printf("teste:%s\n",Chave_M1);
    printf("teste:%s\n",Chave_M2);
    //Preenchimento das Matrizes Za & Zb
    int contador_row = 0;
    int contador_collumn = 0;
    
    //Za -> Filled per Row

    for(int contador_row=0;contador_row < K;contador_row++){
        char temp_m[K_T];
        temp_m[K_T] = '\0';
        memmove(temp_m,Chave_M1,sizeof(temp_m));
        char *rotated_array = rotate(temp_m,contador_row);
        //printf("Testes alt ChaveM: %s\n", Chave_M1);
        //printf("Pre_error_Hello\n");
        //printf("Teste out_of_bounds:%c\n", Chave_M1[15]);
        //printf("Hello\n");
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            if(contador_row == 0){
                Za[contador_row][contador_collumn] = Chave_M1[contador_collumn];
            }else{
                Za[contador_row][contador_collumn] = rotated_array[contador_collumn];
            }
        }
        //printf("%s\n",Za[contador_row]);
    }
    
    //Zb -> Filled per Collumn
    for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
        char temp_m2[K_T];
        temp_m2[K_T] = '\0';
        memmove(temp_m2,Chave_M2,sizeof(temp_m2));
        char *rotated_array2 = rotate(temp_m2,contador_collumn);
        //printf("Collumn Rotation:%s\n",rotated_array2);
        for(int contador_row=0;contador_row < K;contador_row++){
            if(contador_collumn == 0){
                Zb[contador_row][contador_collumn] = Chave_M2[contador_row];
            }else{
                Zb[contador_row][contador_collumn] = rotated_array2[contador_row];
            }
        }
        //printf("%s\n",Zb[contador_collumn]);
    }

    //Zc & Zd -> Filled by random char generated trough the random() function, this random char is calculated from a random number with the original Za or Zb character
    //Of the current position in the matrix as a seed.

    for(int contador_row=0;contador_row < K;contador_row++){
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            //printf("Chamadas Random:%d\n",contador_collumn);
            Zc[contador_row][contador_collumn] = random_char(Za[contador_row][contador_collumn],Min,Max); //Estamos a ficar com chars a mais no array
            Zd[contador_row][contador_collumn] = random_char(Zb[contador_row][contador_collumn],Min,Max);
        }
        /*printf("Iterações:%d\n",contador_row);
        printf("%s\n",Zc[contador_row]);
        printf("Tamanho_array: %d\n",sizeof(Zc[contador_row]));
        printf("D:%s\n",Zd[contador_row]);
        printf("Tamanho_arrayD: %d",sizeof(Zd[contador_row]));*/
    }

    //Z matrix -> Filled by xored elements from all the previous Matrixes. 
    for(int contador_row=0;contador_row < K;contador_row++){
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            printf("Iteração: %d",contador_row);
            unsigned char temp = xor(*Za[contador_row,contador_collumn],
            *Zb[contador_row,contador_collumn],
            *Zc[contador_row,contador_collumn],
            *Zd[contador_row,contador_collumn]);
            Z[contador_row][contador_collumn] = temp;
            printf("Z gerado: %c\n",Z[contador_row][contador_collumn]);
        }
        //printf("Iterações:%d\n",contador_row);
        //printf("%s\n",Zc[contador_row]);
        //printf("Tamanho_array: %d",sizeof(Zc[contador_row]));
    }


    return 0;
}