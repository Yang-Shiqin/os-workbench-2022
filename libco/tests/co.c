#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ucontext.h>
  
#ifndef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug()
#endif
  
#define STACK_SIZE 8192

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
    enum co_status state;
    const char* name;
    ucontext_t ucp;
    char stack[STACK_SIZE];  // 栈太小会segmentation fault
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;
    struct co *    waiter;
};
  
static struct co* list[128]={0};
static int next=0;
static int now=0;
static int max=0;
static struct co end;
  
void co_end(int i){
    list[i]->state = 0;
    debug("end\n");
}
  
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
    getcontext(&(list[now]->ucp));
    debug("yield\n");
    int i = rand() % (max+1);
    while((NULL==list[i]) || ((list[i]->state!=CO_RUNNING) 
        && (list[i]->state!=CO_NEW))){
        i = rand() % (max+1);
    }
    int last=now;
    debug("%d, %d, %d, %s, %d\n", now, i, max, list[i]->name, list[i]->state);
    now = i;
    if(list[now]->state==CO_NEW){
        list[now]->state=CO_RUNNING;
        getcontext(&(list[now]->ucp));
        list[now]->ucp.uc_stack.ss_sp = list[now]->stack;
        list[now]->ucp.uc_stack.ss_size = sizeof(list[now]->stack); // 栈大小
        makecontext(&(list[now]->ucp), (void (*)(void))list[now]->func, 1, list[now]->arg); // 指定待执行的函数入口
        swapcontext(&(list[last]->ucp), &(list[now]->ucp));
        list[now]->state = CO_DEAD;
        list[now]->waiter->state = CO_RUNNING;
        co_yield();
    }else{
        swapcontext(&(list[last]->ucp), &(list[now]->ucp));
    }

}
static __attribute__((constructor)) void co_constructor(void) {
  struct co *current = co_start("main", NULL, NULL);
  current->state = CO_RUNNING; 
}