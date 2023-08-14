#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
    if(argc<2){
        printf("参数错误！");
        exit(0);
    }

    //copy一下参数
    char *argvs[MAXARG];
    for(int i=1;i<argc;i++){
        argvs[i-1]=argv[i];
    }

    char buf[64];
    
    while(1){
        int loc=argc-1;

        //循环读取参数
        //参数添加在argvs的后方
        gets(buf,sizeof(buf));
        if(buf[0]==0){
            break;
        }
        argvs[loc]=buf;
        //将空格替换为尾0
        for(char*p=buf;*p!=0;p++){
            if(*p==' '){
                *p=0;
                argvs[loc++]=p+1;
            }
            else if (*p=='\n'){
                *p=0;
            }
        }
        //调用子程序执行当前行
        if(fork()==0){
            exec(argv[1],argvs);
        }
    }
    wait(0);
    exit(0);
}

