#include "co.h"

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define STACK_SIZE 8192
#define LIST_SIZE 128
static struct co* list[LIST_SIZE]={0};
static int next=0;
static int now=0;
static int max=0;

struct co *current;

enum co_status {
  CO_NEW = 1,  // 新创建，还未执行过
  CO_RUNNING,  // 已经执行过
  CO_WAITING,  // 在 co_wait 上等待
  CO_DEAD,     // 已经结束，但还未释放资源
};

struct co {
  const char *name;
  void (*func)(void *);  // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;            // 协程的状态
  struct co *waiter;                // 是否有其他协程在等待当前协程
  jmp_buf context;                  // 寄存器现场 (setjmp.h)
  unsigned char stack[STACK_SIZE];  // 协程的堆栈
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *coroutine = (struct co *)malloc(sizeof(struct co));
  assert(coroutine);

  coroutine->name = name;
  coroutine->func = func;
  coroutine->arg = arg;
  coroutine->status = CO_NEW;
  coroutine->waiter = NULL;

    list[next] = coroutine;
    while(NULL!=list[next]){
        next = (next+1)%LIST_SIZE;
        max = next>max?next:max;
    }
  return coroutine;
}

void co_wait(struct co *coroutine) {
  assert(coroutine);

  if (coroutine->status != CO_DEAD) {
    coroutine->waiter = list[now];
    list[now]->status = CO_WAITING;
    co_yield ();
  }

    int i;
    for(i=0; i<LIST_SIZE && list[i]!=coroutine; i++){;}
    if(list[i]==coroutine){
        free(coroutine);
        coroutine = NULL;
        list[i]=NULL;
    }
}

/*
 * 切换栈，即让选中协程的所有堆栈信息在自己的堆栈中，而非调用者的堆栈。保存调用者需要保存的寄存器，并调用指定的函数
 */
static inline void stack_switch_call(void *sp, void *entry, void *arg) {
  asm volatile(
#if __x86_64__
      "movq %0, %%rsp; movq %2, %%rdi; call *%1"
      :
      : "b"((uintptr_t)sp), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#else
      "movl %%ecx, 4(%0); movl %0, %%esp; movl %2, 0(%0); call *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#endif
  );
}

#define __LONG_JUMP_STATUS (1)
void co_yield () {
  int status = setjmp(list[now]->context);
  if (!status) {
    int i = rand() % (max+1);
    while((NULL==list[i]) ||
    ((list[i]->status!=CO_RUNNING) && (list[i]->status!=CO_NEW))){
        i = rand() % (max+1);
    }
    now = i;

    assert(list[now]);

    if (list[now]->status == CO_RUNNING) {
      longjmp(list[now]->context, __LONG_JUMP_STATUS);
    } else {
      ((struct co volatile *)list[now])->status = CO_RUNNING;  //这里如果直接赋值，编译器会和后面的覆写进行优化

      // 栈由高地址向低地址生长
      stack_switch_call(list[now]->stack + STACK_SIZE, list[now]->func, list[now]->arg);
      //恢复相关寄存器

      //此时协程已经完成执行
      list[now]->status = CO_DEAD;

      if (list[now]->waiter) {
        list[now]->waiter->status = CO_RUNNING;
      }
      co_yield ();
    }
  }

  assert(status && list[now]->status == CO_RUNNING);  //此时一定是选中的进程通过longjmp跳转到的情况执行到这里
}

static __attribute__((constructor)) void co_constructor(void) {
  list[now] = co_start("main", NULL, NULL);
  list[now]->status = CO_RUNNING;
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