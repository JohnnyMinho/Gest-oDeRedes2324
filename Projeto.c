#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define K 10
//Têmos de criar uma struct para a MIB


char* Gen_Chave_Mestra(int tamanho_chave){
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

char* rotate(char* M, int rotation_number){
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


int main(int argc, char *argv[]) {
    
  //Temos de fazer a leitura do ficheiro para tirar as configurações, têmos também de decidir como fica o formato
    char Za[K][K];
    char Zb[K][K];
    size_t K_T = K;
    //Za[K][K] = '\0';
    //Zb[K][K] = '\0';

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
        char* temp_m = rotate(Chave_MX[0],contador_row);
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
        for(int contador_row=0;contador_row < K;contador_row++){
            if(contador_collumn == 0){
                Zb[contador_row][contador_collumn] = Chave_MX[2][contador_row];
            }else{
                char* temp_m2=rotate(Chave_MX[2],contador_row);
                Zb[contador_row][contador_collumn] = temp_m2[contador_row];
            }
        }
    }
    printf("%s\n",Zb[0]);
    

    return 0;
}