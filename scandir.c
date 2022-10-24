#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

//#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>

//использовано повторно из useless
pid_t launch(char *command);  //запуск процесса параллельно
int getsize(int descr); //размер файла в байтах (символах)
char *copy2mem(int descr, int size); //копирование файла в доступную память, возвращает указатель на эту память
int copystr(char *source, char *target, int length); //копирование строки известной длины

int tcopy(char *source, char *target); //копирование строки без указания длины, до первого \0
int repsym(char* source, char* target, char symbol, char* replacement); //замена символов на последовательность

int main(int argc, char** argv)
{
    char dir[256] = {};
    char outname[256] = {};

    char thisdir[] = " ./";
    strcat(dir, thisdir);

    // разные случаи количества аргументов
    //неправильные
    if(argc < 2)
    {   printf("insufficient arguments\n"); exit(-1); }

    if(strlen(argv[1]) > 255)
            {printf("output file name too long!\n"); exit(-1);}

    tcopy(argv[1], outname);
    strcat(outname, ".txt");

    //без директории
    if(argc < 3)
        printf("\n no directory specified; scanning current. \n");
    //стандартный
    else
        {
        if(strlen(argv[2]) > 255)
            {printf("directory name too long!\n"); exit(-1);}

        strcat(dir, argv[2]);
        }

        //формирование bash команд для запуска
        char print_list[300] = {};
        char generate_txt_list[300]= {};

        strcat(print_list, "ls ");
        strcat(print_list, dir);

         strcat(generate_txt_list, "ls ");
         strcat(generate_txt_list, dir);
         strcat(generate_txt_list, " | grep -e \"\\.txt\" > txts");

        int txtList, output;

        //запуск команд с ожиданием завершения
        launch(print_list);
        launch(generate_txt_list);



        //ожидание исполнения команды. waitpid не работает
        while((txtList = open("txts", O_RDONLY)) <= 0);

        int size;

        while(!(size = getsize(txtList))); //необходимо подождать, пока в файле появятся символы.  сразу после предыдущего while getsize возвращает 0.

        //без этого программа работает каждый второй раз - сначала создаёт файл после попытки чтения и выдаёт ошибку,
        //потом читает файл с прошлого запуска и работает корректно (с теми же аргументами)

        char* txtNames = copy2mem(txtList, size);

        if((txtList = close(txtList) < 0))
            { printf("close(txts) failure \n"); exit(-1);  }
        
        txtNames[size - 1] = '\0'; //избавление от переноса строки в конце

        char* txtPaths = calloc(1, size*10); //файлы с прикреплёнными спереди путями директории

        // к пути к директории приписывается дополнительная / для присоединения к именам файлов,
        // далее путь приписывается в начало аргумента cat (перед именем первого файла не встречается \n)
        strcat(dir, "/"); 
        strcat(txtPaths, dir);   

        int dirlen = strlen(dir);

        //далее все \nы заменяются на " ./path/"

        repsym(txtNames, (txtPaths + dirlen), '\n', dir);
        free(txtNames); //лишняя память

        //составление команды для создание файла с выводом
        char generate_output[2048] = {};

        strcat(generate_output, "cat ");
        strcat(generate_output, txtPaths);
        strcat(generate_output, " > ");
        strcat(generate_output, outname);

        launch(generate_output);

        //char cleanup[] = "rm txts";
        launch("rm txts"); //уничтожение временного файла

        free(txtPaths);
        return 0;
}



//запуск команды (параллельно главному процессу)
pid_t launch(char *command)
{
    pid_t pid = fork();

    // parent
    if (pid != 0)
        return pid;

    execl("/bin/bash", "bash", "-c", command, NULL);

    exit(-1);
}
// копирование строки известной длины
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
// копирование строки неизвестной длины до первого \0
int tcopy(char *source, char *target)
{
    int i = 0;

    for(; source[i]; i++)
        target[i] = source[i];

    target[i] = 0;
    return i;
}
// размер файла в символах
int getsize(int descr)
{
    int size = lseek(descr, 0, SEEK_END);
    lseek(descr, 0, SEEK_SET);
    return size;
}
// запись файла в память
char *copy2mem(int descr, int size)
{
    char *memory = malloc(size);

    assert(read(descr, memory, size));
    return memory;
}
// заменить символ последовательностью
int repsym(char* source, char* target, char symbol, char* replacement)
{
    int size = strlen(replacement);
    int src = 0, trg = 0;

    for(; source[src]; src++)
         {   
        if(source[src] == symbol)
            for(int rep = 0; rep < size; rep++, trg++)
                target[trg] = replacement[rep];
        else
            {
                target[trg] = source[src];
                trg++;
            }
         }
    
    return src;
}

