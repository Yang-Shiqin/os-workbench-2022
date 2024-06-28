/******** todo
 * execve怎么用, 环境变量怎么处理(读手册)
 * strace二进制文件位置
 * CMD带/直接执行, 否则在PATH中查找, 的实现
 * 时间统计
 ************* done
 * pipe怎么用
 * fork用法
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// [ ] todo
// 子进程调用strace CMD [ARG], 输出管道给父进程
int strace(int fd, int argc, char *argv[]){
  char **exec_argv = (char**)malloc(sizeof(char*)*(argc+2));
  int i;
  // 构建参数: CMD以/开头, 则直接执行; 否则在PATH中找
  exec_argv[0] = "strace";
  exec_argv[argc+1] = NULL;
  for (i=0; i<argc; i++){
    exec_argv[i+1] = argv[i];
  }
  char *exec_envp[] = { "PATH=/bin:/usr/bin", NULL, };
  // dup2(fd, STDERR_FILENO);
  // close(STDOUT_FILENO);
  execve("/bin/strace", exec_argv, exec_envp);
  perror("execve");
  exit(EXIT_FAILURE);
}

// [ ] todo
// 父进程读取管道输入, 解析并统计syscall时长, 展示出来
int sperf(int fd){
  char buf[1024]={0};
  ssize_t num_read;
  while ((num_read=read(fd, buf, sizeof(buf)-1)) > 0){
    // 解析并显示时间
    buf[num_read] = 0;
    printf("%saaa\n", buf);
    // printf("[%d] Got: '%s'\n", getpid(), buf);
  }
  close(fd);
  return 0;
}

// [ ] todo
// sperf CMD [ARG]  ->  strace CMD [ARG] | sperf
int main(int argc, char *argv[]) {
  // 1. 判断调用形式
  if (argc<2){
    fprintf(stderr, "Usage: %s <CMD> [ARG]\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // 2. 创建管道
  int pipefd[2];
  if (-1==pipe(pipefd)){
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  // 3. fork进程
  pid_t pid = fork();
  if (-1==pid){
    perror("fork");
    exit(EXIT_FAILURE);
  }else if (0==pid){
    // child: 关闭读口
    close(pipefd[0]);
    strace(pipefd[1], argc-1, argv+1);
  }else{
    // parent: 关闭写口
    close(pipefd[1]);
    sperf(pipefd[0]);
  }




  // char *exec_argv[] = { "strace", "ls", NULL, };
  // char *exec_envp[] = { "PATH=/bin", NULL, };
  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
