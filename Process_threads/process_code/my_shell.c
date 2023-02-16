#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int main()
{
    char command[256]; //用于接收命令
    const char* Myshell = "[Myshell@timer]$:";
    while (1)
    {
        command[0] = 0; //重置命令数组
        printf("%s",Myshell);
        fgets(command,256,stdin);//由键盘输入命令
        command[strlen(command)-1] = '\0';
        

        char* argv[16];//用来传入进程替换函数的数组
        argv[0] = strtok(command, " ");
        
        int i = 1;
        do
        {
            argv[i] = strtok(NULL, " ");
            if(argv[i]==NULL)
                break;
            ++i;
            
        } while (1);

        pid_t id = fork(); //创建子进程，返回子进程ID
        if(id < 0)
        {
            perror("子进程创建失败\n");
            continue;
        }
        if(id==0)
        {
            execvp(argv[0],argv);
            exit(1);
        }
        int st;
        pid_t ret = waitpid(id,&st,0); //父进程等待子进程，阻塞方式
        if(ret == id)
        {
            if((st>>8)&0xff == 1)
            {
                printf("调用失败，有可能是命令输入错误，请重新输入，返回码是%d\n",(st>>8)&0xff);
            }
            else{
                printf("调用成功，返回码是%d\n",(st>>8)&0xff);
            }
        }
    }
    
    return 0;
}
