#include <game.h>

#define SIDE 16
static int w, h;

static void init() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

// 黑白棋盘格
void splash() {
  init();
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {// x和y一奇一偶
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
}

/* ysq */
void get_wh(int* ret_w, int* ret_h){
  *ret_w = w;
  *ret_h = h;
}

void draw_me(int x, int y){
    // x = (x<0?0:x);
    // y = (y<0?0:y);
    // x = ((x+h/16)>w?(w-h/16):x);
    // y = (y>(h*15/16)?(h*15/16):y);

    for (int xi=0; xi<16; xi++){
      for (int yi=0;yi<16;yi++){
        draw_tile(xi*w/16, yi*h/16, w/16, h/16, 0);
      }
    }
    draw_tile(x, y, h/16, h/16, 0xff0000);
}
/* ysq end */
