#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc!=2){
      printf("参数错误!");
    }
    else{
      int sleepTime=atoi(argv[1]);
      sleep(sleepTime);
    }

  exit(0);
}

