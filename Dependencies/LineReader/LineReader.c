#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int HowManyLines(int fd){
    //Devolve o número de linhas total de um ficheiro de texto
    char buffer[2];
    int num_lines = 0;
    while(read(fd,buffer,sizeof(buffer))>0){
        for(int i = 0; i < 2;i++){
            //printf("Buffer:%c\n",buffer[i]);
            if(buffer[i] == '\n'){
                //printf("Buffer:%c",buffer[i]);
                num_lines++;
            }
        }
    }
    lseek(fd,0,SEEK_SET);
    return num_lines;
}

int HowManyCharsLine(char *buff){
    //Returns the number of chars in a line
    int n = 0;
    int chars_in_line = 0;
    while(buff[n] != '\n'){
        n++;
        chars_in_line++;
    }
    return chars_in_line+1;
}

int * LinePosition(int fd, char *arr){
    //Devolve as posições das linhas que contêm uma certa string.
    char buffer[1024];
    int max_num_lines = HowManyLines(fd);
    int *temp_array = malloc(max_num_lines * sizeof(int));
    int how_many_chars_read=0;
    int how_many_chars_read_moment = 0;
    int matches = 0;
    int lines_consumed = 0;
    while(read(fd,buffer,sizeof(buffer))>0){
        while(how_many_chars_read < 1024){
            char aux_buffer[1024];
            memmove(aux_buffer,buffer+how_many_chars_read,(1024-how_many_chars_read));
            printf("HowManyCharsRead:%d\n",how_many_chars_read);
            memset(buffer+(1024-how_many_chars_read),'\0',how_many_chars_read);
            printf("Test1:%s\n",buffer);
            how_many_chars_read_moment = HowManyCharsLine(aux_buffer);
            //printf("HowManyCharsReadM:%d\n",how_many_chars_read_moment);
            how_many_chars_read = how_many_chars_read + how_many_chars_read_moment;
            //printf("Read Chars: %d",how_many_chars_read);
            char temp_buffer[how_many_chars_read_moment];
            temp_buffer[how_many_chars_read_moment] = '\0';
            memcpy(temp_buffer,aux_buffer,how_many_chars_read_moment*sizeof(char));
            //printf("Test1:%s\n",buffer);
            //printf("Aux_Buffer: %s\n",aux_buffer);
           // printf("Test2:%s\n",temp_buffer);
            if(strstr(temp_buffer,arr) != NULL){
                temp_array[matches] = lines_consumed;
                matches++;
               // printf("Match, total matches:%d\n",matches);
            }  
            lines_consumed++;
        }
    }
    lseek(fd,0,SEEK_SET);
    return temp_array;
}


int main(int argc,char *argv[]){
    //Teste Function
    int fd = open("MIB_file.mib",O_RDONLY);
    //int test1;
    //test1 = HowManyLines(fd);
    //printf("Test1: %d\n",test1);
    //LinePosition(fd,"ABABABABABA"); //Objetivo verificar que não encontra nenhum
    LinePosition(fd,"systemRestartDate"); //Objetivo, este deve devolver o mesmo valor que o test1
    close(fd);
}