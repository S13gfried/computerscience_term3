#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define NUMBER 0
#define FILENAME 1

#define BUFFER_SIZE 256
#define MAX_LAUNCHES 256

#define SOURCE_DEFAULT "execsequence.txt"

int str2int(char *string, int length);
int getsize(int descr);
char *copy2mem(int descr, int size);
int copystr(char *source, char *target, int length);

pid_t launch(char *name);

typedef struct filedelay_t
{
    char name[256];
    int time;
} filedelay;

int timesort(filedelay *arr, int size);

int main(int argc, char **argv)
{
    clock_t start = clock(); //get T1

    int filed;
    if (argc > 1)
    {
        if ((filed = open(argv[1], O_RDONLY)) < 0)
        {
            printf("FILE FAILURE\n");
            exit(-1);
        }
    }
    else
    {
        if ((filed = open(SOURCE_DEFAULT, O_RDONLY)) < 0)
        {
            printf("FILE FAILURE\n");
            exit(-1);
        }
    }

    int size = getsize(filed);
    char *filecon = copy2mem(filed, size);

    char buffer[BUFFER_SIZE] = {};

    int countdownIndex = 0;
    int scanmode = NUMBER;

    filedelay roster[MAX_LAUNCHES] = {};
    int rosterSize = 0;
    int time;

    for (int i = 0; i < size; i++)
    {
        if ((filecon[i] == ' '&& scanmode == NUMBER) || filecon[i] == '\n') //!
        {
            //detect end of segment
                if (i - countdownIndex > 255)
                {
                    free(filecon);
                    printf("line %d: buffer overflow - value described by too much characters!\n", rosterSize + 1);
                    exit(-1);
                }

            buffer[i - countdownIndex] = '\0'; //terminate

            if (scanmode == FILENAME) 
            {
                scanmode = NUMBER; //change mode

                copystr(buffer, roster[rosterSize].name, BUFFER_SIZE); //copy string value
                rosterSize++;
            }
            else if (scanmode == NUMBER)
            {
                if (i - countdownIndex > 9)
                {
                    free(filecon);
                    printf("line %d: time is too big!\n", rosterSize + 1);
                    exit(-1);
                }

                scanmode = FILENAME; //change mode

                if((time = str2int(buffer, 9)) == -1) //convert string to int, set value
                {
                    free(filecon);
                    printf("line %d: invalid time value\n", rosterSize + 1);
                    exit(-1);
                }

                roster[rosterSize].time = time;
            }
            for (; filecon[i] == ' ' || filecon[i] == '\n'; i++); //skip empty spaces

            //printf("", i - countdownIndex);
            countdownIndex = i;
            //printf("%d\n", i);
        }
            //load next char into buffer
            buffer[i - countdownIndex] = filecon[i];
    }

    printf("found %d files\n", rosterSize);

    if (rosterSize == 0) // check for trivial case with no launches
        return 0;


    timesort(roster, rosterSize); // sort launches by time


    int deltas[MAX_LAUNCHES] = {}; // delta Ts between launches
    deltas[0] = (roster[0]).time;

    for (int i = 1; i < rosterSize; i++)
        deltas[i] = (roster[i]).time - (roster[i - 1]).time; // calculate deltas

    clock_t end = clock(); //get T2

    int deltaTu = (int)(((end - start)*1000000) / CLOCKS_PER_SEC); //deltaT, microsec

    //printf("%d us elapsed\n", deltaTu); usually takes no more than 200 mcsec

    usleep(deltas[0]*1000000 - deltaTu);
    printf("launching %s...\n", roster[0].name); //correct first delta with deltaT, run program 0

    if (launch((roster[0]).name) == -1)
            printf("fork failed!\n");

    for (int i = 1; i < rosterSize; i++)
    {
        sleep(deltas[i]);
        printf("launching %s...\n", roster[i].name); 

        if (launch((roster[i]).name) == -1)
            printf("fork failed!\n");

    }

    free(filecon); //FREE MEMORY

    return 0;
}

// convert string no longer than LENGTH to int.
int str2int(char *string, int length)
{
    char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int result = 0;
    int match;

    for (int i = 0; i < length; i++)
    {
        if (string[i] == '\n' || string[i] == '\0' || string[i] == ' ')
            break;
            
        result *= 10;

        match = 1;
        for (int j = 0; j < 10; j++)
            if (digits[j] == string[i])
            {
                result += j;
                match = 0;
                break;
            }

        if(match)
            return -1;
    }

    return result;
}

// returns size of file(DESCR) in characters(bytes)
int getsize(int descr)
{
    int size = lseek(descr, 0, SEEK_END);
    lseek(descr, 0, SEEK_SET);
    return size;
}

// allocate a chunk of memory and write file(DESCR) of SIZE size contents into it. 
char *copy2mem(int descr, int size)
{
    char *memory = malloc(size);

    assert(read(descr, memory, size));
    return memory;
}

// launch NAME process
pid_t launch(char *name)
{
    pid_t pid = fork();

    // parent
    if (pid != 0)
        return pid;

    // child

    //char arg[258] = {};

    //strcat(arg, "./");
    //strcat(arg, name);

    execl("/bin/bash", "bash", "-c", name, NULL);

    exit(-1);
}

// sort ARR of SIZE filedelays by their time, in ascending order.
int timesort(filedelay *arr, int size)
{
    filedelay minimum, exchange;
    int minIndex;

    for (int i = 0; i < size; i++)
    {
        minimum = arr[i];
        minIndex = i;

        for (int j = i + 1; j < size; j++)
            if ((arr[j]).time < minimum.time)
            {
                minIndex = j;
                minimum = arr[j];
            }

        exchange = arr[i];
        arr[i] = arr[minIndex];
        arr[minIndex] = exchange;
    }
    return 0;
}

// copy LENGTH long string value from SOURCE to TARGET. returns no. of moved chars.
int copystr(char *source, char *target, int length)
{
    for (int i = 0; i < length; i++)
    {
        target[i] = source[i];
        if(target[i] == 0)
            return i;
    }

    return length;
}
