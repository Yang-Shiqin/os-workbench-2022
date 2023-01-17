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
#define LIST_SIZE 128

static inline void stack_switch_call(void *sp, void *entry, void * arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp), "d"((uintptr_t)entry), "a"((uintptr_t)arg) : "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg) : "memory"
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
    const char* name;
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;
    enum co_status state;
    struct co* waiter;
    jmp_buf env;
    unsigned char stack[STACK_SIZE];  // 栈太小会segmentation fault
};

static struct co* list[LIST_SIZE]={0};
static int next=0;
static int now=0;
static int max=0;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *coroutine = (struct co *)malloc(sizeof(struct co));
  assert(coroutine);

  coroutine->name = name;
  coroutine->func = func;
  coroutine->arg = arg;
  coroutine->state = CO_NEW;
  coroutine->waiter = NULL;

    list[next] = coroutine;
    while(NULL!=list[next]){
        next = (next+1)%LIST_SIZE;
        max = next>max?next:max;
    }
  return coroutine;
}

void co_wait(struct co *co) {
    // todo
    while(CO_DEAD!=co->state){
        list[now]->state = CO_WAITING;
        co->waiter = list[now];
        co_yield();
    }
    int i;
    for(i=0; i<LIST_SIZE && list[i]!=co; i++){;}
    if(list[i]==co){
        free(co);
        co = NULL;
        list[i]=NULL;
    }
}

void co_yield() {
    int val = setjmp(list[now]->env);
    if(0==val){
        int i = rand() % (max+1);
        while((NULL==list[i]) ||
        ((list[i]->state!=CO_RUNNING) && (list[i]->state!=CO_NEW))){
            i = rand() % (max+1);
        }
        now = i;
        if(list[now]->state==CO_NEW){
            ((struct co volatile *)list[now])->state = CO_RUNNING;
            // 寄存器从高向低生长
            stack_switch_call(list[now]->stack+STACK_SIZE, list[now]->func, list[now]->arg);   // 切换栈，在自己的栈上运行函数
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
    list[now] = co_start("main", NULL, NULL);
    list[now]->state = CO_RUNNING;
}

static __attribute__((destructor)) void co_destructor(void) {
    int i;
    for(i=0; i<LIST_SIZE; i++){
        if(list[i]!=NULL){
            free(list[i]);
            list[i]=NULL;
        }
    }
}