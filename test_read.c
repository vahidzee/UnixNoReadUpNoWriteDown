#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int file1;
    char buffer[1024];
    long int num;

    if(((file1 = open(argv[1], O_RDONLY)) == -1)){
        perror("could not read the file");
        return 1;
    }
    while((num=read(file1, buffer, 1024)) > 0){
        printf("%s", buffer);
    }
    close(file1);
}