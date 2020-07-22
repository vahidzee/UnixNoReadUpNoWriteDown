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
    printf("successful\n");
    close(file1);
}