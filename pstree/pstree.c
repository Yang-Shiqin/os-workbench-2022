#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>

// 10进制itoa
#define itoa(buf, n)        snprintf(buf, sizeof(buf), "%d", n)

typedef struct Proc{
    int pid;                // pid
    char comm[16];          // 进程名, 这个不太好
    struct Proc* father;    // 父进程
    struct Proc* brother;   // 同父的兄弟
    struct Proc* child;     // 子进程
}proc;

int is_num(char * str);
proc* gen_sorted_pstree();
void print_pstree(proc* root, int is_p, int tab);

int main(int argc, char *argv[]) {
    int pflag=0, vflag=0;
    int opt, option_index=0;
    char *short_options = "pnV";
    static struct option long_options[] = {
        {"show-pids", 0, 0, 'p'},
        {"numeric-sort", 0, 0, 'n'},
        {"version", 0, 0, 'V'},
        {0, 0, 0, 0},
    };
    
    proc *pr = NULL;
    while((opt=getopt_long(argc, argv, short_options, long_options, &option_index))!=-1){
        switch (opt){
        case 'V':
            fprintf(stderr, "Copyright (C) 2022 YSQ");
            vflag = 1;
            break;
        case 'p':
            pflag = 1;
            break;
        case 'n':
            break;
        default:
            vflag = 1;
            printf("Usage: %s [-p] [-n]\n\
   or: %s -V\n\n\
Display a tree of processes.\n\n\
  -n, --numeric-sort  sort output by PID\n\
  -p, --show-pids     show PIDs\n\
  -V, --version       display version information", argv[0], argv[0]);
            break;
        }
    }
    if(!vflag){
        pr = gen_sorted_pstree();
        if(pflag){
            print_pstree(pr, 1, 0);
        }else{
            print_pstree(pr, 0, 0);
        }
    }
    return 0;
}

proc* gen_sorted_pstree(){
    DIR *dp;
    struct dirent *dirp;
    char dirname[32];
    int pid_arr[128]={0};
    int pid_num=0;
    int i;
    assert((dp=opendir("/proc")));
    while((dirp=readdir(dp))!=NULL){
        if(is_num(dirp->d_name)){
            pid_arr[pid_num++] = atoi(dirp->d_name);
        }
    }
    proc* proc_list = (proc*)malloc((pid_num+1)*sizeof(proc));
    for(i=0; i<pid_num; i++){
        proc_list[i].pid = pid_arr[i];
        char dir[128] = "/proc/";
        char tmp_pid[16]={0};
        itoa(tmp_pid, pid_arr[i]);
        strcat(dir, tmp_pid);
        strcat(dir, "/stat");
        FILE *fp = NULL;
        fp = fopen(dir, "r");
        char str[256] = {0};
        char *tmp_str=NULL;
        fgets(str, 255, fp);
        tmp_str = strtok(str, " ");
        tmp_str = strtok(NULL, " ");
        tmp_str[strlen(tmp_str)-1] = 0;
        strcpy(proc_list[i].comm, &tmp_str[1]);
        tmp_str = strtok(NULL, " ");
        tmp_str = strtok(NULL, " ");
        int ppid = atoi(tmp_str);
        if(ppid){
            int j;
            for(j=0; j<i; j++){     // 前提：父进程pid小于子进程pid
                if(proc_list[j].pid == ppid){
                    proc_list[i].father = &proc_list[j];
                    if(NULL==proc_list[j].child){
                        proc_list[j].child = &proc_list[i];
                    }else{
                        proc *tmp_proc = proc_list[j].child;
                        while(tmp_proc->brother) tmp_proc = tmp_proc->brother;
                        tmp_proc->brother = &proc_list[i];
                    }
                }
            }
        }
    }
    return &proc_list[0];
}

// 递归打印
void print_pstree(proc* root, int is_p, int tab){
    int i;
    if(root){
        for(i=0; i<tab; i++) printf("  ");
        if(is_p)
            printf("%s(%d)\n", root->comm, root->pid);
        else
            printf("%s\n", root->comm);
        print_pstree(root->child, is_p, tab+1);
        print_pstree(root->brother, is_p, tab);
    }
}


int is_num(char * str){
    int i;
    for(i=0; i<strlen(str); i++){
        if (str[i]<'0' || str[i]>'9') return 0;
    }
    return 1;
}
