#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "file.h"
#include "disk.h"


//get command from input
int getcmd(char *buf, int nbuf){
    printf("@ ");
    memset(buf, 0, nbuf);
    scanf("%[^\n]", buf);
    getchar();      // get enter in buffer
    buf[strlen(buf)] = '\0';
  	return 0;
}

//parse the input buffer into argv
int getargs(char* buf, char* argv[]){
	int argc = 0;
	int i = 0;
    while(buf[i] != '\0')
	{
        //跳过空格
		while(buf[i] == ' ')
			i++;
		if(buf[i] == '\0')
			break;
		argv[argc++] = buf + i;
		while(buf[i] != ' ' && buf[i] != '\0')
			i++;
		if(buf[i] == '\0')
			break;
		buf[i++] = '\0';
	}
	argv[argc] = 0;
	return argc;
}

int exec_ls(char* argv[], int argc){
    int inode_num;
    if(argc == 1)
        inode_num = open_by_path("/");
    else if(argc == 2){
        if(argv[1][strlen(argv[1]) - 1] != '/')
            strcat(argv[1], "/");
        inode_num = open_by_path(argv[1]);
    }else{
        printf("only  \'ls\' or \'ls path\' /n");
        return 0;
    }
    if(inode_num < 0){
        printf("Can't open path \'%s\'\n", argv[1]);
        return 0;
    }
    dir_list(inode_num);
    return 0;
}


int exec_touch(char* argv[], int argc){
    char *p;
    int inode_num;
    if(argc != 2){
        printf("only \'touch /path/fliename \'\n");
        return 0;
    }

    p = argv[1] + strlen(argv[1]) - 1;
    while(p >= argv[1] && *p != '/')
        p--;
    if(p >= argv[1]){
        *p = 0;
        inode_num = open_by_path(argv[1]);
    }else
        inode_num = open_by_path("/");
    p++;
    if(!fd_create(inode_num, p, 0))
        printf("Create file failed!\n");
    return 0;
}

int exec_mkdir(char* argv[], int argc){
    char *p;
    int inode_num;
    if(argc != 2){
        printf("only \'mkdir /path/directoryname \'\n");
        return 0;
    }

    p = argv[1] + strlen(argv[1]) - 1;
    if(*p == '/')
        *p = 0;
    while(p >= argv[1] && *p != '/')
        p--;
    if(p >= argv[1]){
        *p = 0;
        inode_num = open_by_path(argv[1]);
    }else
        inode_num = open_by_path("/");
    p++;
    if(!fd_create(inode_num, p, 1))
        printf("Create directory failed!\n");
    return 0;
}

int exec_cp(char* argv[], int argc){
    int src_num;
    int dst_num;
    char *dst_name;

    if(argc != 3){
        printf("only \'cp /path/filename /path/filename  \'\n");
        return 0;
    }

    dst_name = argv[2] + strlen(argv[2]) - 1;
    while(dst_name >= argv[2] && *dst_name != '/')
            dst_name--;
    if(*dst_name == '/'){
        *dst_name = 0; 
        dst_num = open_by_path(argv[2]);
    }
    else
        dst_num = open_by_path("/");   
    dst_name++;
    src_num = open_by_path(argv[1]);
    if(file_copy(src_num, dst_num, dst_name) < 0){
        printf("copy file failed\n");
        return 0;
    }
    return 0;
}

int exec_shutdown(){
    close_disk();
    printf("file system closed\n");
    return 1;
}

int runcmd(char* argv[], int argc){
    if(argc==0)
        return 0;
    else if(!strcmp(argv[0], "ls"))
        return exec_ls(argv, argc);   
    else if(!strcmp(argv[0], "mkdir"))
        return exec_mkdir(argv, argc);         
    else if(!strcmp(argv[0], "touch"))
        return exec_touch(argv, argc);
    else if(!strcmp(argv[0], "cp"))
        return exec_cp(argv, argc);
    else if(!strcmp(argv[0], "shutdown"))
        return exec_shutdown();    
    else{
        printf("%s is invalid command\n", argv[0]);
        return 0;
    }
}

void fs_shell(){
    char buf[MAXBUF];
    char* argv[MAXARGS];
    int argc;
    // Read and run input commands.
  	while(getcmd(buf, sizeof(buf)) >= 0){
		argc = getargs(buf, argv);
		if(runcmd(argv, argc))
            break;
  	}
}