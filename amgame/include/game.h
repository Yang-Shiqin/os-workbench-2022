#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

void splash();
int  move();
void get_wh(int* ret_w, int* ret_h);
void draw_me(int x, int y);
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

// 打印10进制int
static inline void puti(int num){
    char str[10]={0};
    int i=0;
    while(num){
        str[i++] = (num%10)+'0';
        num /= num;
    }
    for(;i>=0;i--){
        putch(str[i]);
    }
}
