#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

void splash();
/* ysq */
int  move();
void get_wh(int* ret_w, int* ret_h);
void draw_me(int x, int y);

// 打印10进制int
void put_int(int num);
/* ysq end */
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

