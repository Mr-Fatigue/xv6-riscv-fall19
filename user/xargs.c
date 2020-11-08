#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

void main(int argc , char* argv[]){
    if(argc < 2){
        printf("Missing arguments");
        exit();
    }
    char *newProcessArgv[MAXARG];
    for(int i = 0 ; i < argc-1 ; i++){//读取前面的参数
        newProcessArgv[i] = argv[i+1];
    }
    int argvCounter = argc - 1;//之前已经输入了argc-1个参数，因此下一个参数应当输入argc-1处
    char temp[40];//用于读取标准输入
    char* pointer = temp;//用于标记标准输入读取到哪里
    while(read(0,pointer,sizeof(char))){//read读到EOF返回值为0
        if(*pointer == ' '){//前面读取的算作一个参数，因此下一个存储的参数应当从头开始读
            *pointer = '\0';
            newProcessArgv[argvCounter] = (char*)malloc(40*sizeof(char));
            strcpy(newProcessArgv[argvCounter],temp);
            argvCounter++;
            pointer = temp;
        }
        else if(*pointer == '\n'){//已经换行，应当exec并重新计算参数个数
            *pointer = '\0';
            newProcessArgv[argvCounter] = (char*)malloc(40*sizeof(char));
            strcpy(newProcessArgv[argvCounter],temp);
            if(fork() == 0){
                exec(newProcessArgv[0],newProcessArgv);
            }
            wait();
            argvCounter = argc - 1;
            pointer = temp;
        }
        else{//还在读取一个参数中
            pointer++;
        }
    }
    exit();
}