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
    // struct SyscallInfo * next;
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

// // 父进程读取管道输入, 解析并统计syscall时长, 展示出来
// int sperf(int fd){
//   char buf[256] = {0};
//   char *pbuf = buf;
//   char name_buf[16] = {0};  // \nsyscall_name(
//   char time_buf[16] = {0};  // <syscall_time>
//   double time=0, total_time=0;
//   int reading_name = 1, reading_time = 0, reading = 1; // 1是正在读取(read没读完整), 0是没有正在读取
//   SyscallInfo syscall_info_list[512] = {0};
//   int tail=0, i;
//   ssize_t num_read;
//   while ((num_read=read(fd, buf, sizeof(buf)-1)) > 0){
//     pbuf = buf;
//     // 解析并显示时间
//     buf[num_read] = 0;
//     // printf("%s", buf);
//     while(pbuf<buf+num_read){
//       assert(!(reading_name&reading_time&1)); // 不能同时正在读取
//       assert(reading>=(reading_name|reading_time));
//       char * pi = NULL;
//       if (reading_name){// 读syscall name
//         pi = strstr(pbuf, "(");
//         if(pi){  // 读完syscall name
//           *pi = 0;
//           strcat(name_buf, pbuf);
//           reading_name = 0;
//           pbuf = pi+1;
//         }else{  // syscall name中断
//           strcat(name_buf, pbuf);
//           pbuf = buf+num_read;
//         }
//       }else if (reading_time){
//           pi = strstr(pbuf, ">");
//           if (!pi){  // syscall time中断
//             strcat(time_buf, pbuf);
//             pbuf = buf+num_read;
//           }else{  // 读完一条syscall
//             reading_time = 0;
//             *pi = 0;
//             strcat(time_buf, pbuf);
//             int list_i;
//             double tmp_time = atof(time_buf);
//             time += tmp_time;
//             for (list_i=0; list_i<tail; list_i++){
//               if (0==strcmp(syscall_info_list[list_i].name, name_buf)){
//                 syscall_info_list[list_i].time += tmp_time;
//                 break;
//               }
//             }
//             if (list_i==tail){
//               strcpy(syscall_info_list[tail].name, name_buf);
//               syscall_info_list[tail].time = tmp_time;
//               tail++;
//               assert(tail<512);
//             }
//             name_buf[0] = 0;
//             time_buf[0] = 0;
//             reading = 0;
//             pbuf = pi+1;
//             if (time>0.1){
//               total_time += time;
//               time = 0;
//               qsort(syscall_info_list, tail, sizeof(SyscallInfo), compare); // 排序
//               printf("Time: %.1fs\n", total_time);
//               for (i = 0; i < (tail < 5 ? tail : 5); i++) {
//                   // printf("%s: %.6f seconds\n", syscall_info_list[i].name, syscall_info_list[i].time);
//                   printf("%s (%d%%)\n", syscall_info_list[i].name, (int)(syscall_info_list[i].time*100/total_time));
//               }
//               for (i=0; i<80; i++){
//                 putc(0, stdout);
//               }
//               printf("==================\n");
//             }
//           }
//       }else if (reading){ // 读完name还没开始读time
//         pi = strstr(pbuf, "<");
//         if(pi){
//           reading_time = 1;
//           pbuf = pi+1;
//         }else{
//           pbuf = buf+num_read;
//         }
//       }else{
//         pi = strstr(pbuf, "\n");
//         if (pi){
//           reading = 1;
//           reading_name = 1;
//           pbuf = pi+1;
//         }else{
//           pbuf = buf+num_read;
//         }
//       }
//     }
//   }
//   // 最后一次打印
//   total_time += time;
//   time = 0;
//   qsort(syscall_info_list, tail, sizeof(SyscallInfo), compare); // 排序
//   printf("Time: %.1fs\n", total_time);
//   for (i = 0; i < (tail < 5 ? tail : 5); i++) {
//       // printf("%s: %.6f seconds\n", syscall_info_list[i].name, syscall_info_list[i].time);
//       printf("%s (%d%%)\n", syscall_info_list[i].name, (int)(syscall_info_list[i].time*100/total_time));
//   }
//   for (i=0; i<80; i++){
//     putc(0, stdout);
//   }
//   printf("==================\n");
//   close(fd);
//   return 0;
// }

void remove_quoted_contents(const char *input, char *output) {
  const char *pattern = "\"(\\\\\"|[^\"])*\"";  // 匹配两个双引号之间的内容
  regex_t regex;
  regmatch_t match;
  // int start = 0;
  int end = 0;
  int offset = 0;
  int ret;

  // 编译正则表达式
  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    perror("regex");
    exit(EXIT_FAILURE);
  }

  // 逐个匹配并删除
  while ((ret = regexec(&regex, input + offset, 1, &match, 0)) == 0) {
    // start = match.rm_so + offset;
    end = match.rm_eo + offset;

    // 将匹配前的内容复制到输出
    strncpy(output + strlen(output), input + offset, match.rm_so);
    // 复制双引号到输出
    strcat(output, "\"\"");
    offset = end;
  }

  // 复制剩余的内容
  strcat(output, input + offset);

  // 释放正则表达式对象
  regfree(&regex);
}

int sperf(int fd){
  const char *pattern = "([a-zA-Z_]+)\\(.*\\)\\s*=.*<([0-9.]+)>";
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
  printf("1sdfsdf\n");
    remove_quoted_contents(remove_buf[0], remove_buf[1]); // 去除引号内的内容
  printf("2sdfsdf\n");
    pbuf = remove_buf[1];
    printf("pbuf:%s\n\n", pbuf);
    while(*pbuf!=0 && (pbuf2 = strstr(pbuf, "\n"))!=NULL){
  printf("3sdfsdf\n");
      ret = regexec(&regex, pbuf, 3, matches, 0);
  printf("4sdfsdf\n");
      if (!ret) {
  printf("5sdfsdf\n");
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
        pbuf = pbuf2+1;
      } else if (ret == REG_NOMATCH) {
        printf("No match\n");
      } else {
        char errbuf[100];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Regex match failed: %s\n", errbuf);
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
