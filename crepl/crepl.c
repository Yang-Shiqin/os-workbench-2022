// BUG: 重复定义函数, 或者定义错误, 会把原来的文件覆盖掉(因为是编译后才判断对错的), 调用是调用的新定义的(但是作业没要求我们这些错误输入必须实现对)
// TODO: 错误信息可改进
// 目前功能: 合法输入应该都行

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

#define FNAME_LEN_MAX (64)
#define ROOT "/tmp/crepl/"
#define SO ROOT "all.so"

// 解析函数名: int name(xxx){xxx};
int get_func_name(const char *line, char *dst){
  const char *pattern = "int\\s+([a-zA-Z_]\\w*)\\s*\\(";  // 可改进
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
    if (len>=FNAME_LEN_MAX-16){
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

// 创建函数文件或匿名函数文件(name==NULL), 生成链接所有.c的动态链接库
int create_func(const char* name, const char *line, char *files_name[], int* tail){
  int pipefd[2];
  if (-1==pipe(pipefd)){
    perror("pipe");
    return -1;
  }
  pid_t pid=fork();
  if (-1==pid){
    perror("fork");
    return -1;
  }else if (pid==0){  // child, 写
    close(pipefd[0]);
    dup2(pipefd[1], STDERR_FILENO); // 把写口连到stderr
    close(STDOUT_FILENO);
    FILE *file = NULL;
    char file_name[FNAME_LEN_MAX] = {ROOT}; // 匿名函数名
    char *args[128] = {"gcc", "-fPIC", "-shared", "-w", "-o", SO};  // -w屏蔽warning
    int argc;
    for (argc=0; argc < *tail; argc++) {
      args[argc+6] = files_name[argc];
    }
    argc += 6;
    args[argc] = NULL;
    strcat(file_name, "-.c");
    if (NULL==name) name = file_name; // 表达式, name指向filename
    file = fopen(name, "w");
    if (file == NULL) {
      printf("Error opening file\n");
      return -1;
    }
    if (file_name==name){  // 表达式, 创建匿名函数
      fprintf(file, "int ____________________(){ return %s;}", line);
      args[argc++] = file_name;
      args[argc] = NULL;
    }else{
      fprintf(file, "%s", line);
    }
    fclose(file);
    // 动态链接: 表达式是为了调用, 函数是为了判断这个文件有没有问题
    execvp("gcc", args);
    perror("execlp");
    return -1;
  }else{  // parent, 读
    close(pipefd[1]);
    char buf[64] = {0};
    if ((read(pipefd[0], buf, sizeof(buf)-1)) > 0){ // 输出stderr就算报错
      {printf("%s\n", buf);}while((read(pipefd[0], buf, sizeof(buf)-1)) > 0);
      return -1;
    }
    wait(NULL);
  }
  return 0;
}

// 调用匿名函数
int call_func(int *res){
  char *error;
  int (*func)()=NULL;
  void *handle = dlopen(SO, RTLD_LAZY); // 显示调用动态链接库
  if ((error=dlerror())!=NULL){
    fprintf(stderr, "%s\n", error);
    return -1;
  }
  func = (int(*)())dlsym(handle, "____________________"); // 匿名函数
  if ((error=dlerror())!=NULL){
    fprintf(stderr, "%s\n", error);
    return -1;
  }
  *res = func();
  dlclose(handle);
  return 0;
}

// 解析一行
int parse(const char * line, char *files_name[], int* tail){
  assert(line);
  char name[FNAME_LEN_MAX]={ROOT};  // 太长不要
  while(*line==' ') line++;
  if (*line=='\n') return 0;
  if (strstr(line, "int ")==line){  // 函数
    // 1. 检查并获取函数命名
    if(0==get_func_name(line, name+strlen(name))){ // 没有检查有没有重复(题目里说没重复定义, 遇到可以算undefine behavior)
      strcat(name, ".c");
      files_name[(*tail)++] = strdup(name); // 每解析出一个名称, 就malloc字符串内存报错下来
      // 2. 创建函数
      if(-1==create_func(name, line, files_name, tail)){
        free(files_name[*tail]);
        (*tail)--; // 失败则去掉当前函数
      }
    }
  }else{  // 表达式
    // 1. 创建匿名函数
    if(-1!=create_func(NULL, line, files_name, tail)){
      int res;
      // 2. 调用匿名函数
      call_func(&res);
      printf("%d\n", res);
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  static char line[4096];
  char* files_name[128] = {0};    // 最多100个函数
  int tail=0;
  int result = mkdir(ROOT, 0755);
  if (result != 0) {
    if (errno != EEXIST) {
      printf("Error creating directory: %d\n", errno);
      return 1;
    }
  }
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {  // ctrl-d则不会获取字符并退出, 否则至少获得/n字符
      break;
    }
    parse(line, files_name, &tail);
  }
  // free strdup的内存
  for (; tail>0; tail--){
    free(files_name[tail-1]);
  }
  return 0;
}
