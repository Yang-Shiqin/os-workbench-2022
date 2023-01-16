#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>

#define STACK_SIZE 2048

#ifndef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug()
#endif

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
  );
}

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
    enum co_status state;       // 协程状态
    char* name;                 // 协程名字
    jmp_buf env;                // 保存上下文
    uint8_t stack[STACK_SIZE];  // 协程的堆栈
    void (*func)(void *);       // co_start 指定的入口地址和参数
    void *arg;    
    struct co* waiter;          // 是否有其他协程在等待当前协程
};

struct co* list[128]={0};
int now=0;
int next=0;
int max=0;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    debug("start\n");
    struct co* ret = malloc(sizeof(struct co));
    list[next] = ret;
    now = next;
    while(NULL!=list[next]){
        next = (next+1)%128;
        max = next>max?next:max;
    }

    ret->state = CO_NEW; // 开始
    ret->name = name;
    ret->func = func;
    ret->arg = arg;
    ret->waiter = NULL;
    func(arg);
    return ret;
}

void co_wait(struct co *co) {
    while(NULL!=co && CO_DEAD!=co->state){
        co_yield();
    }
    if(NULL!=co){
        // 不对，我咋知道后面有没有人等我
        free(co);
        co = NULL;
        list[now]=NULL;
    }    
}

void co_yield() {
    debug("yield\n");
    int i = rand() % (max+1);
    while(NULL==list[i]){
        i = rand() % (max+1);
    }
    int tmp=now;
    debug("%d, %d, %d\n", now, i, max);
    now = i;
    int val = setjmp(list[tmp]->env);
    if (val == 0) {
        tack_switch_call(list[now]->stack, list[now]->func, list[now]->arg);
        longjmp(list[now]->env, 0);
    } else {
        ;
    }
}
