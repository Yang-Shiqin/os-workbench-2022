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

// static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
//   asm volatile (
// #if __x86_64__
//     "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
//       : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
// #else
//     "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
//       : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
// #endif
//   );
// }
/*
 * 切换栈，即让选中协程的所有堆栈信息在自己的堆栈中，而非调用者的堆栈。保存调用者需要保存的寄存器，并调用指定的函数
 */
static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile(
#if __x86_64__
      "movq %0, %%rsp; movq %2, %%rdi; call *%1"
      :
      : "b"((uintptr_t)sp - 16), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#else
      "movl %%ecx, 4(%0); movl %0, %%esp; movl %2, 0(%0); call *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#endif
  );
}
/*
 * 从调用的指定函数返回，并恢复相关的寄存器。此时协程执行结束，以后再也不会执行该协程的上下文。这里需要注意的是，其和上面并不是对称的，因为调用协程给了新创建的选中协程的堆栈，则选中协程以后就在自己的堆栈上执行，永远不会返回到调用协程的堆栈。
 */
static inline void restore_return() {
  asm volatile(
#if __x86_64__
      "movq 0(%%rsp), %%rcx"
      :
      :
#else
      "movl 4(%%esp), %%ecx"
      :
      :
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
    char stack[STACK_SIZE];  // 栈太小会segmentation fault
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
    debug("start\n");
    struct co* ret = malloc(sizeof(struct co));
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