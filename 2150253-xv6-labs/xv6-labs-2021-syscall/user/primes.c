#include "kernel/types.h"
#include "user/user.h"

#define MAX_NUM 35


void 
prime(int *p)
{
    int temp;
    int n;
    close(p[1]);
    if (read(p[0],&temp,sizeof(temp))!=sizeof(temp)){
        exit(0);
    }


    printf("prime %d\n",temp);

    if (read(p[0], &n, 4) == sizeof(temp)){
        int newp[2];
        pipe(newp);
        if(fork()==0){
            //子进程
            prime(newp);
        }
        else{
            //父进程
            close(newp[0]);
            if(n%temp!=0){
                write(newp[1],&n,sizeof(n));
            }
            while(read(p[0], &n, 4)==sizeof(n)){
                if(n%temp!=0){
                    write(newp[1],&n,sizeof(n));
                }
            }
            close(p[0]);
            close(newp[1]);
            wait(0);
        }
    }
    exit(0);
}


int 
main(int argc,char* argv[])
{
    //定义初始管道
    int p[2];
    pipe(p);
    if(fork()==0){
        //子进程
        prime(p);
    }
    else{
        //父进程
        //依次传出即可
        close(p[0]);
        for(int i=2;i<=MAX_NUM;i++){
            write(p[1],&i,sizeof(i));
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}

