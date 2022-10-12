#include <game.h>

// Operating system is a C program!
int main(const char *args) {
  int x=0, y=0;
  int w,h;
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");
  put_int(123);

  get_wh(&w, &h);
  splash();
  draw_me(x, y);
  put_int(w);
  puts("\n");
  put_int(h);

  puts("Press any key to see its key code...\n");
  while (1) {
    // print_key();
    int tmp = move();
    if (tmp&0x10){
        halt(0);
    }
    if (tmp){
        puts("\nysq");
      x += ((tmp&1) - (!!(tmp&2)))*2;
      y += ((!!(tmp&4)) - (!!(tmp&8)))*2;
      if (x+h/16>w){
          puts("\nerr1");
      }
      x = (x<0?0:x);
      if (y>(h*15/16)){
          puts("\nerr2");
      }
      y = (y<0?0:y);
      //x = ((x+h/16)>w?(w-h/16):x);
      //y = (y>(h*15/16)?(h*15/16):y);
      draw_me(x, y);
    }
  }
  return 0;
}
