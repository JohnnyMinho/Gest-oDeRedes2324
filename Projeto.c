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
    char* temp_Return = M;
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
    return M;
}

unsigned char random_char(char seed,int inc, int max){
    srand(seed);
    unsigned char Pos_random=(char)('0' + rand() % 255);
    printf("Pos_random gerado = %d\n",Pos_random);
    return Pos_random;
}

int main(int argc, char *argv[]) {
    
  //Temos de fazer a leitura do ficheiro para tirar as configurações, têmos também de decidir como fica o formato
   unsigned char Za[K][K];
   unsigned char Zb[K][K];
   unsigned char Zc[K][K];
   unsigned char Zd[K][K];
    size_t K_T = K;

    

   /*for(int contador_row=0;contador_row != K;contador_row++){
        for(int contador_collumn=0;contador_collumn != K;contador_collumn++){
            printf("Teste_NUm:%d",contador_collumn);
            Za[K][K] = '\0';
            Zb[K][K] = '\0';
            Zc[K][K] = '\0';
            Zd[K][K] = '\0';
        }
    }*/ 

    char* Chave_Mestra = Gen_Chave_Mestra(2*K); //Continuamos a devolver como array ou como int?

    char Chave_MX[2][K];
    Chave_MX[0][K] = '\0';
    Chave_MX[1][K] = '\0';
    Chave_MX[2][K] = '\0';
    /*for(int N_chaves = 0;N_chaves<2;N_chaves++){
        printf("N_chaves:%d\n",N_chaves);
        if(N_chaves == 0){
            memmove(Chave_MX[N_chaves],Chave_Mestra,K*sizeof(char)); //Ficamos com os 2 pares de chaves necessários
        }else{
            memmove(Chave_MX[N_chaves],Chave_Mestra+((N_chaves)*K),K*sizeof(char)); //Ficamos com os 2 pares de chaves necessários
        }
    }*/
    for(int N_chaves = 0;N_chaves<2;N_chaves++){
        for(int i = 0 +(10*N_chaves);i<(10+(10*N_chaves));i++){
            Chave_MX[N_chaves][i] = Chave_Mestra[i];
        }
    }
    printf("K:%d",K);
    printf("Chave_Mestra:%s\n",Chave_Mestra);
    printf("teste:%s\n",Chave_MX[0]);
    printf("teste:%s\n",Chave_MX[2]);
    //Preenchimento das Matrizes Za & Zb
    int contador_row = 0;
    int contador_collumn = 0;
    
    //Za -> Filled per Row

    for(int contador_row=0;contador_row < K;contador_row++){
        unsigned char* temp_m = rotate(Chave_MX[0],contador_row);
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            if(contador_row == 0){
                Za[contador_row][contador_collumn] = Chave_MX[0][contador_collumn];
            }else{
                Za[contador_row][contador_collumn] = temp_m[contador_collumn];
            }
        }
        printf("%s\n",Za[contador_row]);
    }
    
    //Zb -> Filled per Collumn
    for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
        unsigned char* temp_m2=rotate(Chave_MX[2],contador_collumn);
        for(int contador_row=0;contador_row < K;contador_row++){
            if(contador_collumn == 0){
                Zb[contador_row][contador_collumn] = Chave_MX[2][contador_row];
            }else{
                Zb[contador_row][contador_collumn] = temp_m2[contador_row];
            }
        }
    }

    //Zc -> Filled by random numbers generated trough the random() function.

    for(int contador_row=0;contador_row < K;contador_row++){
        for(int contador_collumn=0;contador_collumn < K;contador_collumn++){
            printf("Chamadas Random:%d\n",contador_collumn);
            Zc[contador_row][contador_collumn] = random_char(Za[contador_row][contador_collumn],Min,Max); //Estamos a ficar com chars a mais no array
        }
        printf("Iterações:%d\n",contador_row);
        printf("%s\n",Zc[contador_row]);
        printf("Tamanho_array: %d",sizeof(Zc[contador_row]));
    }
    //Zd -> Filled by

    return 0;
}