#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bunny.h"

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int uint64_t;

typedef struct display_regs_t display_regs;
__attribute__((packed)) struct display_regs_t {
  volatile uint32_t ctrl;
  volatile uint8_t *addr;
  volatile uint32_t width;
  volatile uint32_t height;
};

typedef struct grayframer_regs_t grayframer_regs;
__attribute__((packed)) struct grayframer_regs_t {
  volatile uint32_t ctrl;
  volatile uint8_t *addr;
  volatile uint32_t in_pos;
  volatile uint32_t out_pos;
  volatile uint32_t size;
};

typedef struct keyboard_regs_t keyboard_regs;
__attribute__((packed)) struct keyboard_regs_t {
  volatile uint32_t data;
};

const uint32_t width = 320;
const uint32_t height = 240;
volatile uint8_t *videomem = (uint8_t *)0x50000000;
volatile display_regs *vid = (display_regs *)0x80050000;
volatile grayframer_regs *gf = (grayframer_regs *)0x80050200;
volatile keyboard_regs *kb = (keyboard_regs *)0x80050300;

void loadimage(uint8_t *image, volatile uint8_t *address, uint32_t xpos, uint32_t ypos, uint32_t video_width, uint32_t video_height, uint32_t frame_width, uint32_t frame_height) {
    int i;
    for(i=0;i<video_height;i++) {
        memcpy(
                (void *)&address[(ypos*video_width*2)+xpos+(i*video_width*4)],
                (void *)image+(i*video_width*2),
                video_width*2
                );
    }
}

int main(int argc, char *argv[]) {

  vid->addr = videomem;
  vid->width = width*2;
  vid->height = height*2;
  vid->ctrl |= 0x3;

  gf->addr = videomem;
  gf->in_pos = 0;
  gf->out_pos = (width << 16) | 0;
  gf->size = (width*2 << 16) | height*2;
  gf->ctrl |= 0x3;

  loadimage(bunny_orig_png,videomem,0,0,width,height,width*2,height*2);
  // allocate memory and copy image
  uint8_t *imgcopy;
  imgcopy = (uint8_t *)malloc(100);
  //loadimage(bunny_orig_png,imgcopy,0,0,width,height,width,height);
  printf("imgcopy addr: %x",*imgcopy);

  while(1) {
    if (kb->data) printf("sw got key: %d\n",kb->data);
    vid->ctrl |= 0x2;
    gf->ctrl |= 0x2;

  }
}
