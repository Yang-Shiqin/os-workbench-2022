#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define FNAME_LEN_MAX (32)

int get_func_name(const char *line, char *dst){
  const char *pattern = "int\\s+([a-zA-Z_]\\w*)\\s*\\(";
  regex_t regex;
  regmatch_t matches[2];
  // 编译正则表达式
  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    printf("Failed to compile regular expression\n");
    return -1;
  }
  // 执行正则匹配
  if (regexec(&regex, line, 2, matches, 0) == 0) {
    // 提取匹配到的函数名
    int len = matches[1].rm_eo-matches[1].rm_so;
    if (len>=30){
      regfree(&regex);
      return -1;
    }
    strncpy(dst, line+matches[1].rm_so, len);
    dst[len] = 0;
    regfree(&regex);
    return 0;
  } else {
    printf("No match found\n");
    regfree(&regex);
    return -1;
  }
}

// [ ] todo: 管道, gcc错误则删掉文件, 错误信息需要管道来传递
// 创建函数文件, 若创建匿名函数则生成链接所有.c的动态链接库
int create_func(const char* name, const char *line, char *files_name[], int* tail){
  // fork?
  pid_t pid=fork();
  if (-1==pid){
    perror("fork");
    return -1;
  }else if (pid==0){
    FILE *file = NULL;
    char file_name[64] = {"/tmp/crepl/"};
    char *args[128] = {"gcc", "-fPIC", "-shared", "-o", "all.so"};
    int argc;
    for (argc=0; argc < *tail; argc++) {
      args[argc+5] = files_name[argc];
    }
    argc += 5;
    args[argc] = NULL;
    if (NULL==name){  // 匿名文件, 用mkstemp生成临时文件
      strcat(file_name, "anonXXXXXX");
      int fd = mkstemp(file_name);
      if (fd == -1) {
        perror("mkstemp");
        return -1;
      }
      write(fd, line, strlen(line));
      // 完成操作后关闭文件描述符
      close(fd);
      args[argc++] = file_name;
      args[argc] = NULL;
      // 动态链接: 为了调用
      execvp("gcc", args);
      // // 删除临时文件
      // unlink(file_name);
    }else{
      // 写文件
      file = fopen(name, "w");
      if (file == NULL) {
        printf("Error opening file\n");
        return -1;
      }
      fprintf(file, "%s", line);
      fclose(file);
      // 编译成动态链接库: 为了判断这个文件有没有问题
      execvp("gcc", args);
      (*tail)--;  // 失败则删掉这个文件
    }
    perror("execlp");
    return -1;
  }else{
    wait(NULL);
    return 0;
  }
  return 0;
}

int parse(const char * line, char *files_name[], int* tail){
  assert(line);
  // [ ] todo: 绝对地址
  char name[64]={"/tmp/crepl/"};  // 太长不要
  while(*line==' ') line++;
  if (*line=='\n') return 0;
  if (strstr(line, "int ")==line){  // 函数
    // 1. 检查并获取函数命名
    if(0==get_func_name(line, name+strlen(name))){ // 没有检查有没有重复(题目里说没重复定义, 遇到可以算undefine behavior)
      strcat(name, ".c");
      files_name[(*tail)++] = strdup(name);
      // 2. 创建函数
      create_func(name, line, files_name, tail);
    }
    printf("func: Got %zu chars.\n", strlen(line)); // ??
  }else{  // 表达式
    // 1. 创建匿名函数
    create_func(NULL, line, files_name, tail);
    // 2. [ ] todo: 调用匿名函数
    printf("expr: Got %zu chars.\n", strlen(line)); // ??
  }
  return 0;
}

int main(int argc, char *argv[]) {
  static char line[4096];
  char* files_name[128] = {0};    // 最多100个函数
  int tail=0;
  int result = mkdir("/tmp/crepl", 0755);
  if (result != 0) {
    if (errno != EEXIST) {
      printf("Error creating directory: %d\n", errno);
      return 1;
    }
  }
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    parse(line, files_name, &tail);
  }
}
