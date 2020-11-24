#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"


void parsecmd(char* buf,int *argc,char *argv){//解析当前的命令用作分词,只以换行和空格作为结尾
    int counter = 0;
    int flag = 0;//作为buf上的指针     
    while(buf[flag] == ' ') flag++;//先去除前面所有的空格
    for(; *argc < 15 ; (*argc)++){//逐个进行参数解析
        counter = 0;
        while(buf[flag] != ' ' && buf[flag] != '\0' && buf[flag] != '\n' && counter < 39){
            argv[(*argc)*40+counter] = buf[flag];
            counter++;
            flag++;
        }
        argv[(*argc)*40+counter] = '\0';//补上字符串的结尾
        while(buf[flag] == ' ') flag++;//跳过之后的所有空格
        if(buf[flag] == '\0' || buf[flag] == '\n'){//说明读完了全部的读入，可以输入结束符
            (*argc)++;
            argv[(*argc)*40]  = '\0';
            break;
        }
    }
}

void runcmd(int argc,char *argv){
    int p[2];//作为管道使用
    int type = 0;//0表示此为标准命令，1表示本指令中含有管道操作符
    int flag = 0;
    for(int i = 0 ; i<=argc ; i++){//先查找是否具有管道操作符，如果有，则处理第一个|
        if(strcmp(argv+i*40,"|") == 0){
            type = 1;
            argv[40*i] = '\0';
            flag = i;//标记从哪个位置出现第一个|
            break;
        }
    }
    if(type == 0){ 
        flag = argc;//如果含pipe操作符，则进入另一个分支进行处理；如果不含pipe操作符，则应当扫描完全部参数
        for(int i=0;i<=argc;i++){
            if(strcmp(argv+i*40,">") == 0){
                close(1);
                open(argv+(i+1)*40,O_CREATE|O_WRONLY);//创建并只写打开
                argv[40*i]= ' ';
                argv[40*(i+1)]=' ';
            }
        }
        for(int i=0;i<=argc;i++){
            if(strcmp(argv+i*40,"<") == 0){
                close(0);
                open(argv+(i+1)*40,O_RDONLY);//只读打开
                argv[40*i]=' ';
                argv[40*(i+1)]=' ';
            }
        }
        int realArgc = 0;
        char realArgv[16][40];
        char *realArgvPointer[16];
        for(int i = 0 ; i < 16 ; i++){
            realArgvPointer[i] = realArgv[i];
        }
        for(int i=0;i<=argc;i++){//重新整理处理过的部分
            if(argv[40*i] == ' '){//当某一个的开头为空格时，说明这个并不是参数，而是受过处理后的重定向，因此不做处理
                continue;
            }
            else if(argv[40*i] == 0){//说明读取到了本条命令的尾部,将最后一个参数标为空指针
                realArgvPointer[realArgc]=0;
                break;
            }
            else{//拷贝参数
                strcpy(realArgv[realArgc],argv+40*i);
                realArgc++;
            }
        }
        exec(realArgvPointer[0],realArgvPointer);
    }
    else{
        pipe(p);
        if(fork()==0){//管道左方的指令
            close(1);
            dup(p[1]);//重定向输出文件
            close(p[0]);
            close(p[1]);
            runcmd(flag,argv);
            exit(0);
        }
        
        if(fork()==0){//管道右方的指令
            close(0);
            dup(p[0]);//重定向输入文件
            close(p[0]);
            close(p[1]);
            runcmd(argc-flag-1, argv+40*(flag+1));//参数应当出去前面的flag个外加一个0，只剩argc-flag-1个
            exit(0);
        }
        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
    }
    exit(0);
}

int getcmd(char *buf, int nbuf)//获得一组输入
{
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}


int main(void){
    static char buf[640];
    static char argv[16][40];
    static int argc = 0;
    int fd;
    while ((fd = open("console", O_RDWR)) >= 0)
    {
        if (fd >= 3)
        {
            close(fd);
            break;
        }
    }
    while(getcmd(buf,sizeof(buf))>=0){
        if(buf[0] == 'c' && buf[1] == 'd' && buf[2]==' '){
            buf[strlen(buf)-1] = 0;
            if(chdir(buf+3)<0){
                fprintf(2,"cannot cd %s\n",buf+3);
            }
            continue;
        }
        if(fork() == 0){
            parsecmd(buf,&argc,argv[0]);//将命令解析为分词的形式
            runcmd(argc,argv[0]);
        }
        wait(0);
        memset(argv[0],0,640);
        argc = 0;
    }
    exit(0);
}