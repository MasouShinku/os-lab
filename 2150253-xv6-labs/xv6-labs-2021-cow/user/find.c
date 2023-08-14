#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


//找到最后的文件名
char*
fmtname(char *path)
{
    char *p = path;
    while (*p)
        p++;
    while (*p != '/' && p != path)
        p--;
    return p == path ? p : ++p;
}


void
DFS(char *path,char* fName)
{
    //DFS搜索所有文件
    //变量及处理参照ls.c
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:
            //为文件时，查看是否同名
            if(strcmp(fmtname(path),fName)==0){
                printf("%s\n",path);
            }
            break;
        case T_DIR:
            //为文件夹时，先判断长度是否合法
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd,&de,sizeof(de))==sizeof(de)){
                if(de.inum==0||strcmp(de.name,".")==0||strcmp(de.name,"..")==0)
                    continue;
                memmove(p,de.name,DIRSIZ);
                p[DIRSIZ]=0;
                DFS(buf,fName);
            }
            break;

    }
    close(fd);
}

int 
main(int argc, char *argv[])
{
    if(argc!=3){
        printf("参数错误！");
        exit(0);
    }
    DFS(argv[1],argv[2]);
    exit(0);
}
