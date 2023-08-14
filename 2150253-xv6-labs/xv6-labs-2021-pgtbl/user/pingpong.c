#include "kernel/types.h"
#include "user/user.h"


int 
main(int argc,char *argv[])
{
    //定义管道的读写端口
    //p1:父进程->子进程，p2:子进程->父进程
    //[0]为生产者，[1]为消费者
    int p1[2],p2[2];
    pipe(p1);
    pipe(p2);
    char buf[1];
 
    if(fork()==0){//子进程
        //关闭无用端口
        close(p2[0]);
        close(p1[1]);
        read(p1[0],buf,1);
        printf("%d: received ping\n",getpid());
        write(p2[1],buf,1);
        close(p2[1]);
    }
    else{//父进程
        //关闭无用端口
        close(p1[0]);
        close(p2[1]);
        write(p1[1],buf,1);
        close(p1[1]);
        read(p2[0],buf,1);
        printf("%d: received pong\n",getpid());
        wait(0);
    }
    exit(0);
}
