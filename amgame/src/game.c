#include <game.h>

// Operating system is a C program!
int main(const char *args) {
  int x=0, y=0;
  int w,h;
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();
  // ysq
  get_wh(&w, &h); // 要在splash之后(splash的init获取w和h)
  draw_me(x, y);

  //puts("Press any key to see its key code...\n");
  while (1) {
    // print_key();
    
    // ysq
    int tmp = move();
    if (tmp&0x10){
        halt(0);
    }
    if (tmp){
      x += ((tmp&1) - (!!(tmp&2)))*4;
      y += ((!!(tmp&4)) - (!!(tmp&8)))*4;
      x = (x<0?0:x);
      y = (y<0?0:y);
      x = ((x+h/16)>w?(w-h/16):x);
      y = (y>(h*15/16)?(h*15/16):y);
      draw_me(x, y);
    }
  }
  return 0;
}
