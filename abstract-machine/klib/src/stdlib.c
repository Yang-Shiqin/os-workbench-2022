#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int itoa(int n, char *buf, int redix){
    char stk[34] = {0};
    int flag = (n>0)*2-1;
    int top=0;
    int tail=0;
    if (0==n){
        buf[0] = '0';
        buf[1] = 0;
        return 1;
    }
    while(n){
        int tmp = (n%redix)*flag;
        if (tmp>9){
            stk[top++] = tmp-10+'a';
        }else{
            stk[top++] = tmp+'0';
        }
        n /= redix;
    }
    if (-1==flag) buf[tail++] = '-';
    while (top--){
        buf[tail] = stk[top];
        tail++;
    }
    buf[tail] = 0;
    return tail;
}

unsigned int utoa(unsigned int n, char *buf, int redix){
    char stk[34] = {0};
    int top=0;
    int tail=0;
    if (0==n){
        buf[0] = '0';
        buf[1] = 0;
        return 1;
    }
    while(n){
        int tmp = n%redix;
        if (tmp>9){
            stk[top++] = tmp-10+'a';
        }else{
            stk[top++] = tmp+'0';
        }
        n /= redix;
    }
    while (top--){
        buf[tail] = stk[top];
        tail++;
    }
    buf[tail] = 0;
    return tail;
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  panic("Not implemented");
#endif
  return NULL;
}

void free(void *ptr) {
  panic("Not implemented");
}

#endif
