#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <string

#include <dirent.h>

int getDirCatalogue(char* path);

#define BACKUP_LOG "log.txt" 

int main(int argc, char** argv)
{
    if(argc != 3)
        { printf("Invalid arguments\n");  exit(-1); }

    char* source = argv[1];
    char* destination = argv[2];

    getDirCatalogue(source);
}

int getDirCatalogue(char* path)
{
    pid_t rfork = fork();

    if(rfork < 1)
        return rfork;

    
    execl("/bin/bash", "bash", "-c", "cat dircont.txt < ls -l ./", NULL);

    return -1;
}