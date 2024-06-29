#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

// [ ] todo: name可能超, 目前用的是栈上数组, 也可能超
typedef struct SyscallInfo{
    char name[16];  // syscall name
    double time;    // add up duration(s)
} SyscallInfo;

// 自定义比较函数
int compare(const void *a, const void *b) {
    SyscallInfo *s1 = (SyscallInfo *)a;
    SyscallInfo *s2 = (SyscallInfo *)b;
    // 按照 score 字段的降序排列
    if ((s2->time-s1->time < 1e-6) && (s2->time-s1->time>-1e-6)){
      return 0;
    }else if (s2->time > s1->time){
      return 1;
    }else{
      return -1;
    }
}

// 子进程调用strace CMD [ARG], 输出管道给父进程
int strace(int fd, int argc, char *argv[]){
  char **exec_argv = (char**)malloc(sizeof(char*)*(argc+3));
  int i;
  // 构建参数
  exec_argv[0] = "strace";
  exec_argv[1] = "--syscall-times=ns";
  exec_argv[argc+2] = NULL;
  for (i=0; i<argc; i++){
    exec_argv[i+2] = argv[i];
  }
  // execve会自动在环境变量中查找
  char *exec_envp[] = { "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", NULL, };
  dup2(fd, STDERR_FILENO);
  close(STDOUT_FILENO);
  execve("/bin/strace", exec_argv, exec_envp);
  perror("execve");
  exit(EXIT_FAILURE);
}

void remove_quoted_contents(const char *input, char *output) {
  const char *pattern = "\"(\\\\\"|[^\"])*\"";  // 匹配两个双引号之间的内容
  regex_t regex;
  regmatch_t match;
  int end = 0;
  int offset = 0;
  int ret;
  int len=0;
  *output = 0;
  assert(strlen(output)==0);
  // 编译正则表达式
  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    perror("regex");
    exit(EXIT_FAILURE);
  }
  // 逐个匹配并删除
  while ((ret = regexec(&regex, input + offset, 1, &match, 0)) == 0) {
    end = match.rm_eo + offset;
    // 将匹配前的内容复制到输出
    assert(strlen(output)<512);
    len = strlen(output);
    strncpy(output + len, input + offset, match.rm_so);
    *(output + len+match.rm_so+1) = 0;
    // 复制双引号到输出
    offset = end;
  }
  // 复制剩余的内容
  strcat(output, input + offset);
  // 释放正则表达式对象
  regfree(&regex);
}

int sperf(int fd){
  const char *pattern = "([a-zA-Z_0-9]+)\\(.*\\)\\s*=.*<([0-9.]+)>";
  regex_t regex;
  regmatch_t matches[3];
  char buf[256] = {0};
  char name_buf[16] = {0};
  char time_buf[16] = {0};
  char remove_buf[2][512] = {0};
  char *pbuf = NULL;
  char *pbuf2 = NULL;
  double time=0, total_time=0;
  SyscallInfo syscall_info_list[512] = {0};
  int tail=0, i;
  ssize_t num_read;
  // 编译正则表达式
  int ret = regcomp(&regex, pattern, REG_EXTENDED);
  if (ret) {
    perror("regex");
    exit(EXIT_FAILURE);
  }
  while ((num_read=read(fd, buf, sizeof(buf)-1)) > 0){
    buf[num_read]=0;
    strcat(remove_buf[0], buf);
    remove_quoted_contents(remove_buf[0], remove_buf[1]); // 去除引号内的内容
    pbuf = remove_buf[1];
    // fprintf(stderr, "buf:%s\n\n0:%s\n\n1:%s\n\n", buf, remove_buf[0], pbuf);
    while(*pbuf!=0 && (pbuf2 = strstr(pbuf, "\n"))!=NULL){
      ret = regexec(&regex, pbuf, 3, matches, 0);
      if (!ret) {
        // 提取系统调用名称
        int len = matches[1].rm_eo - matches[1].rm_so;
        assert(len<16);
        strncpy(name_buf, pbuf + matches[1].rm_so, len);
        name_buf[len] = '\0';
        // 提取时间
        len = matches[2].rm_eo - matches[2].rm_so;
        assert(len<16);
        strncpy(time_buf, pbuf + matches[2].rm_so, len);
        time_buf[len] = '\0';

        int list_i;
        double tmp_time = atof(time_buf);
        time += tmp_time;
        for (list_i=0; list_i<tail; list_i++){
          if (0==strcmp(syscall_info_list[list_i].name, name_buf)){
            syscall_info_list[list_i].time += tmp_time;
            break;
          }
        }
        if (list_i==tail){
          strncpy(syscall_info_list[tail].name, name_buf, sizeof(name_buf));
          syscall_info_list[tail].time = tmp_time;
          tail++;
          assert(tail<512);
        }
        if (time>0.1){
          total_time += time;
          time = 0;
          qsort(syscall_info_list, tail, sizeof(SyscallInfo), compare); // 排序
          printf("Time: %.1fs\n", total_time);
          for (i = 0; i < (tail < 5 ? tail : 5); i++) {
            // printf("%s: %.6f seconds\n", syscall_info_list[i].name, syscall_info_list[i].time);
            printf("%s (%d%%)\n", syscall_info_list[i].name, (int)(syscall_info_list[i].time*100/total_time));
          }
          for (i=0; i<80; i++){
            putc(0, stdout);
          }
          printf("==================\n");
        }
        // pbuf = pbuf2+1;  // 坑: syscall中间有回车, 因此应该从匹配结尾继续
        pbuf += matches[0].rm_eo+1;
        assert(*(pbuf)!='\n');
      } else if (ret == REG_NOMATCH) {
        break;
      } else {
        perror("regex");
        exit(EXIT_FAILURE);
      }
    }
    if (*pbuf) strncpy(remove_buf[0], pbuf, sizeof(remove_buf[0]));
  }
  // 释放正则表达式对象
  regfree(&regex);
  // 最后一次打印
  total_time += time;
  time = 0;
  qsort(syscall_info_list, tail, sizeof(SyscallInfo), compare); // 排序
  printf("Time: %.1fs\n", total_time);
  for (i = 0; i < (tail < 5 ? tail : 5); i++) {
      // printf("%s: %.6f seconds\n", syscall_info_list[i].name, syscall_info_list[i].time);
      printf("%s (%d%%)\n", syscall_info_list[i].name, (int)(syscall_info_list[i].time*100/total_time));
  }
  for (i=0; i<80; i++){
    putc(0, stdout);
  }
  printf("==================\n");
  close(fd);
  return 0;
}


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
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
