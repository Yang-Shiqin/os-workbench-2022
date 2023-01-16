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
  
struct co {
    char state;
    char name[16];
    ucontext_t ucp;
    ucontext_t ucp_end;
    ucontext_t ucp_sta;
    char stack[8192];  // 栈太小会segmentation fault
    char stack_end[8192];
    char stack_sta[8192];
};
  
static struct co* list[128]={0};
static int next=0;
static int now=0;
static int max=0;
static struct co end;
  
static __attribute__((constructor)) void co_constructor(void) {
  struct co *current = co_start("main", NULL, NULL);
}

void co_end(int i){
    list[i]->state = 0;
    debug("end\n");
}
  
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    debug("start\n");
    struct co* ret = malloc(sizeof(struct co));
    list[next] = ret;
    now = next;
    while(NULL!=list[next]){
        next = (next+1)%128;
        max = next>max?next:max;
    }

    ret->state = 1; // 开始
    getcontext(&(ret->ucp_end));
    getcontext(&(ret->ucp));
    getcontext(&(ret->ucp_sta));
    ret->ucp_end.uc_stack.ss_sp = ret->stack_end;
    ret->ucp_end.uc_stack.ss_size = sizeof(ret->stack_end); // 栈大小
    ret->ucp_end.uc_link = &(ret->ucp_sta);
    makecontext(&(ret->ucp_end), (void (*)(void))co_end, 1, now);
    strcpy(ret->name, name);
    ret->ucp.uc_stack.ss_sp = ret->stack;


    ret->ucp.uc_stack.ss_size = sizeof(ret->stack); // 栈大小
    ret->ucp.uc_link = &(ret->ucp_end);


    ret->ucp_sta.uc_stack.ss_sp = ret->stack_sta;
    ret->ucp_sta.uc_stack.ss_size = sizeof(ret->stack_sta); // 栈大小
    ret->ucp_sta.uc_link = NULL;
    makecontext(&(ret->ucp), (void (*)(void))func, 1, arg); // 指定待执行的函数入口
    debug("before set\n");
    swapcontext(&(ret->ucp_sta), &(ret->ucp));
    debug("after set\n");
    return ret;

}

void co_wait(struct co *co) {
    while(NULL!=co && 1==co->state){
        co_yield();
    }
    if(NULL!=co){
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
    swapcontext(&(list[tmp]->ucp), &(list[i]->ucp));
}