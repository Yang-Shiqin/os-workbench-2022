#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

struct co {
    char state;
    char name[8];
    ucontext_t ucp;
    ucontext_t ucp_end;
    char stack[1024];
};

struct co* list[128]={0};
int next=0;
int now=0;
struct co end;

void co_end(int i){
    list[i]->state = 0;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    ucontext_t context;
    struct co* ret = malloc(sizeof(struct co));
    list[next] = ret;
    now = next;
    while(NULL!=list[next]){
        next = (next+1)%128;
    }

    ret->state = 1; // 开始
    getcontext(&(ret->ucp_end));
    makecontext(&(ret->ucp_end), co_end, 1, now);
    strcpy(ret->name, name);
    ret->ucp.uc_stack.ss_sp = ret->stack;
    ret->ucp.uc_stack.ss_size = sizeof(ret->stack); // 栈大小
    ret->ucp.uc_link = &(ret->ucp_end); 
    getcontext(&(ret->ucp));
    makecontext(&(ret->ucp), func, 1, arg); // 指定待执行的函数入口
    getcontext(&context);
    context.uc_link = &(ret->ucp);
    setcontext(&context);
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
