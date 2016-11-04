#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

typedef struct camera_regs_t camera_regs;
__attribute__((packed)) struct camera_regs_t {
  volatile uint32_t ctrl;
  volatile uint8_t *video_addr;
  volatile uint32_t pos;
  volatile uint32_t size;
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

volatile uint32_t count = 0;
const uint32_t width = 320;
const uint32_t height = 240;
volatile uint8_t *videomem = (uint8_t *)0x50000000;
volatile display_regs *vid = (display_regs *)0x80050000;
volatile camera_regs *cam = (camera_regs *)0x80050100;
volatile grayframer_regs *gf = (grayframer_regs *)0x80050200;
volatile keyboard_regs *kb = (keyboard_regs *)0x80050300;

volatile uint32_t r = 100;
int main(int argc, char *argv[]) {

  int i=0;

  cam->ctrl = (width << 16);
  cam->video_addr = videomem;
  cam->pos = 0; //(0 << 16) | 240;
  cam->size = (width*2 << 16) | height*2; // height*2???

  vid->addr = videomem;
  vid->width = width*2;
  vid->height = height*2;
  vid->ctrl |= 0x3;

  gf->addr = videomem;
  gf->in_pos = 0; //(0 << 16) | height;
  gf->out_pos = (width << 16) | 0;
  gf->size = (width*2 << 16) | height*2;
  gf->ctrl |= 0x3;

  cam->ctrl = (width << 16) | 0x3;

  while(1) {
    cam->ctrl |= 0x3;
  }
}
