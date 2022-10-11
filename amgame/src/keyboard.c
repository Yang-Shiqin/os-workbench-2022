#include <game.h>

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = {
  AM_KEYS(KEYNAME)
};

// 打印接受到的按键
void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

// move, the return value has 4 useful bits each means whether
// move up, down, left and right
int move(){
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
      switch (event.keycode){
      case 0x01: // esc
          return 0x10;
      case 0x4d: // up
          return 8;
      case 0x4e: // down
          return 4;
      case 0x4f: // left
          return 2;
      case 0x50: // right
          return 1;
      default:
          return 0;
      }    
  }
  return 0;
}
