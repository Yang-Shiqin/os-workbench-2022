#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
  
#ifndef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug()
#endif
  
#define STACK_SIZE 8192

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
    enum co_status state;
    const char* name;
    jmp_buf env;
    unsigned char stack[STACK_SIZE];  // 栈太小会segmentation fault
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;
    struct co* waiter;
};
  
static struct co* list[128]={0};
static int next=0;
static int now=0;
static int max=0;
static struct co end;
  
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co* ret = (struct co*)malloc(sizeof(struct co));
    list[next] = ret;
    while(NULL!=list[next]){
        next = (next+1)%128;
        max = next>max?next:max;
    }

    ret->state = CO_NEW; // 开始
    ret->name = name;
    ret->waiter = NULL;
    ret->func = func;
    ret->arg = arg;
    return ret;
}

void co_wait(struct co *co) {
    while(NULL!=co && CO_DEAD!=co->state){
        list[now]->state = CO_WAITING;
        co->waiter = list[now];
        co_yield();
    }
    if(NULL!=co){
        int i;
        for(i=0; i<128 && list[i]!=co; i++){;}
        free(co);
        co = NULL;
        list[i]=NULL;
    }    
}

void co_yield() {
    int val = setjmp(list[now]->env);
    if(0==val){
        int i = rand() % (max+1);
        while((NULL==list[i]) || ((list[i]->state!=CO_RUNNING) 
            && (list[i]->state!=CO_NEW))){
            i = rand() % (max+1);
        }
        // if(i==now) return;
        int last=now;
        now = i;
        if(list[now]->state==CO_NEW){
            list[now]->state=CO_RUNNING;
            // 寄存器从高向低生长
            stack_switch_call(list[now]->stack+STACK_SIZE, list[now]->func, (uintptr_t)list[now]->arg);   // 切换栈，在自己的栈上运行函数
            // 函数运行完
            list[now]->state = CO_DEAD;
            if(list[now]->waiter)
                list[now]->waiter->state = CO_RUNNING;
            co_yield();
        }else{
            longjmp(list[now]->env, 1);
        }
    }
}

static __attribute__((constructor)) void co_constructor(void) {
    struct co *current = co_start("main", NULL, NULL);
    current->state = CO_RUNNING; 
}

static __attribute__((destructor)) void co_destructor(void) {
    int i;
    for(i=0; i<128; i++){
        if(list[i]!=NULL){
            free(list[i]);
            list[i]=NULL;
        }
    }
}