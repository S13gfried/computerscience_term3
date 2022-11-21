#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define MAX_NAME_LENGTH 256
#define TIMESTAMP_LENGTH 12
#define MAX_TREE_DEPTH 5

#define ROSTER_PATH "_ROSTER.txt"
#define BACKUP_DATA_PATH

#define ERRCHECK(expr, msg) \
if(!((expr)))                 \
{                       \
    printf((msg));           \
    exit(-1);               \
}

struct dtree
{
    char name[MAX_NAME_LENGTH];
    char date[TIMESTAMP_LENGTH];
    int branches; //-1 for regular file, 0+ for directories
    struct dtree* branch;
    struct dtree* root;
};

char* revcat(char* to, char* from);
char* gotoword(char* src, int row, int col);
char* strmov(char* to, char* from, int length);
int strcom(char* one, char* two, int length);
int linecount(char* str);
int getsize(int fd);
char* loadfile(int fd, int size);
int traverse(struct dtree* root, int func(struct dtree* root));

char* getfullpath(struct dtree* targetnode, char* buffer);
int createroster(struct dtree* targetnode);
int appenddirs(struct dtree* targetnode);
int generatetree(struct dtree* root);

int main()
{
//generate tree
    //get table
    //find dirs
//compare dates/absence with old roster

    return 0;
}

//Reverse concatenation
char* revcat(char* to, char* from)
{
    char buf[MAX_NAME_LENGTH*2] = {};
    //static char buf[MAX_NAME_LENGTH*2];

    //for(int i = 0; i < MAX_NAME_LENGTH*2; i++)
    //    buf[i] = '\0'; // buffer cleanup

    strcat(buf, from);
    strcat(buf, to);
    strcpy(to, buf);

    return to;
}

//Get pointer to given word in given column (skip Nrow '\n's and Ncolumn ' 's)
char* gotoword(char* src, int row, int col)
{
    int index = 0;

    int rowc = 0;
    int colc = 0; //encountered so far

    for(; rowc < row; index++)
    {
        if(src[index] = '\0')
            return (src + index); //out-of-range case

        if(src[index] == '\n')
            rowc++;
    } //find row

    for(; colc < col; index++)
    {
        if((src[index] == '\0') || (src[index] == ' '))
            return (src + index); //out-of-range case

        if(src[index] == ' ')
            {
                colc++;
                while(src[index + 1] == ' ')
                    index++; //skip multiple
            }
    }
    return (src + index);   
}


//Move strings with known length regardless of '\0's placed
char* strmov(char* to, char* from, int length)
{
    int i = 0;

    for(; i < length; i++)
        to[i] = from[i];

    return to;
}


//Comparison of two strings with known length
int strcom(char* one, char* two, int length)
{
    for(int i = 0; i < length; i++)
        if(one[i] != two[i])
            return 0;
        else if((one[i] == '\0') && (two[i] == '\0')) //forced end of string
            break;

    return 1;
}


//Count lines in a string
int linecount(char* str)
{
    if(str[0] == '\0')
        return 0; //empty string

    int count = 1;
    for(int i = 0; ; i++)
        if(str[i] == '\0')
            break;
        else if(str[i] == '\n')
            count++;
    
    return count;
}

//Get file size in characters
int getsize(int fd)
{
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return size;
}


//Allocate memory and load file contents (+ '\0')
char* loadfile(int fd, int size)
{
    char* mem = malloc(size + 1);
    assert(mem); //errcheck
    assert(read(fd, mem, size));
    mem[size] = '\0';

    return mem;
}


//Recursive tree traversal
int traverse(struct dtree* root, int func(struct dtree* root))
{
    int sum = func(root); //apply function to root

    for(int i = 0; i <= root->branches; i++)
        sum += traverse(&(root->branch[i]), func); //apply given function to branches recursively

    return sum;
}

// ~
char* getfullpath(struct dtree* targetnode, char* buffer)
{
    while(targetnode->root) //ascending to root
        {
        revcat(buffer, "/");
        revcat(buffer, targetnode->name); //append name to the beginning of path
        targetnode = targetnode->root; //get one step higher
        }
    return buffer; //placeholder
}

//generate a roster of files in target node
int createroster(struct dtree* targetnode)
{
    char buf[ MAX_TREE_DEPTH * (MAX_NAME_LENGTH + 1) + 1];
    char command[(MAX_TREE_DEPTH * (MAX_NAME_LENGTH + 1))*2 + 10];
    
    for(int i = 0; i < MAX_TREE_DEPTH * (MAX_NAME_LENGTH + 1); i++)
        buf[i] = '\0'; //buffer cleanup

    for(int i = 0; i < (MAX_TREE_DEPTH * (MAX_NAME_LENGTH + 1))*2 + 10; i++)
        command[i] = '\0';

    getfullpath(targetnode, buf); //get path to current node

    strcat(command, "ls -l ");
    strcat(command, buf);
    strcat(command, " > ");
    strcat(command, buf);
    strcat(command, ROSTER_PATH);

    execl("/bin/bash", "bash", "-c", command, NULL);
    return 0;
}


// Append directories to a branch (ASSUMING A ROSTER FILE AND A TREE NODE ARE PRESENT)
int appenddirs(struct dtree* targetnode)
{
    char path[MAX_TREE_DEPTH * (MAX_NAME_LENGTH + 1)] = {};
    getfullpath(targetnode, path);

    strcat(path, ROSTER_PATH);
    int rosterdesc = open(path, O_RDONLY | 0666);

    int size = getsize(rosterdesc);
    char* roster = loadfile(rosterdesc, size);
    close(rosterdesc); //roster is now stored in memory (alternatively, mmap can be used)

    int entries = linecount(roster); //count entries
    int directories = 0;

    targetnode->branches = entries;
    targetnode->branch = calloc(entries, sizeof(struct dtree)); //create branches

    for(int i = 0; i < entries; i++) //init branches
    {
        char* nameptr = gotoword(roster, i, 8);

        if(strcom(nameptr, ROSTER_PATH, strlen(ROSTER_PATH) + 1)) 
            continue; //do not include the roster file itself
        
        for(int k = 0; (nameptr[k] != '\n') && (nameptr[k] != '\0'); k++) 
            targetnode->branch[i].name[k] = nameptr[k]; //set name

        char* timeptr = gotoword(roster, i, 5);
        strmov(targetnode->branch[i].date, timeptr, TIMESTAMP_LENGTH); //set timestamps


        targetnode->branch[i].root = targetnode; //set root

        
        if(*gotoword(roster, i, 0) == 'd') //assign 0 to "branches" for directories and -1 for non-directories
            {
            targetnode->branch[i].branches = 0;
            directories++;
            }
        else 
            targetnode->branch[i].branches = -1;
    }
    free(roster);
    return directories;
}


//Generate directory tree and rosters recursively
int generatetree(struct dtree* root)
{
    int dirs = 1;
    createroster(root);
    appenddirs(root); //init roster and next layer

    for(int i = 0; i < root->branches; i++)
        if(root->branch[i].branches == 0)
            dirs += generatetree(&(root->branch[i]));
    return dirs;
}
