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
    char stack[4096];
    char stack_end[4096];
    char stack_sta[4096];
};

static struct co* list[128]={0};
static int next=0;
static int now=0;
static struct co end;

void co_end(int i){
    list[i]->state = 0;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    debug("start\n");
    ucontext_t context;
    struct co* ret = malloc(sizeof(struct co));
    list[next] = ret;
    now = next;
    while(NULL!=list[next]){
        next = (next+1)%128;
    }

    ret->state = 1; // 开始
    getcontext(&(ret->ucp_end));
    getcontext(&(ret->ucp));
    getcontext(&(ret->ucp_sta));
    ret->ucp_end.uc_stack.ss_sp = ret->stack_end;
    ret->ucp_end.uc_stack.ss_size = sizeof(ret->stack_end); // 栈大小
    debug("%lu", sizeof(ret->stack_end));
    makecontext(&(ret->ucp_end), (void (*)(void))co_end, 1, now);
    strcpy(ret->name, name);
    ret->ucp.uc_stack.ss_sp = ret->stack;
    ret->ucp.uc_stack.ss_size = sizeof(ret->stack); // 栈大小
    ret->ucp.uc_link = &(ret->ucp_end); 
    ret->ucp_sta.uc_stack.ss_sp = ret->stack_sta;
    ret->ucp_sta.uc_stack.ss_size = sizeof(ret->stack_sta); // 栈大小
    makecontext(&(ret->ucp), (void (*)(void))func, 1, arg); // 指定待执行的函数入口
    // getcontext(&context);
    // context.uc_link = &(ret->ucp);
    debug("before set\n");
    // setcontext(&context);
    // setcontext(&(ret->ucp));
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
    int i = rand() % 128;
    while(NULL==list[i]){
        i = rand() % 128;
    }
    int tmp=now;
    now = i;
    swapcontext(&(list[tmp]->ucp), &(list[i]->ucp));
}
