#include <game.h>

// Operating system is a C program!
int main(const char *args) {
  int x=0, y=0;
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();
  draw_me(x, y);

  puts("Press any key to see its key code...\n");
  while (1) {
    // print_key();
    int tmp = move();
    if (tmp&0x10){
        halt(0);
    }
    if (tmp){
      x += (tmp&1) - (!!(tmp&2));
      y += (tmp&4) - (!!(tmp&8));
      draw_me(x, y);
    }
  }
  return 0;
}
