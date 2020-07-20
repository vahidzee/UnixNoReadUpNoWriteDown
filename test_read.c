#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int file1;
    int file2;
    char buffer[1024];
    long int num;

    if(((file1 = open(argv[1], O_RDONLY)) == -1) || ((file2=open(argv[2],O_CREAT|O_WRONLY|O_TRUNC, 0700)) == -1)){
        perror("could not open either file to copy");
        return 1;
    }

    while((num=read(file1, buffer, 1024)) > 0){
        if(write(file2, buffer, num) != num){
            perror("could not write to destination file");
            return 2;
        }
    }
    close(file1);
    close(file2);
}