// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdisplay
/// @{
/// @file yuv_viewer.cpp
///
///
/// @date 2013-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Rolf Meyer
///
#include "models/ahbdisplay/yuv_viewer.h"
#include "core/common/verbose.h"

SDLYuvViewer::SDLYuvViewer() throw(SDLException)
  : width(YUV_VIEWER_DEFAULT_WIDTH), height(YUV_VIEWER_DEFAULT_HEIGHT) {
  // create SDL surface
  screen = create_screen(YUV_VIEWER_DEFAULT_WIDTH, YUV_VIEWER_DEFAULT_HEIGHT);
  flipcnt = 0;
}

SDLYuvViewer::SDLYuvViewer(uint32_t width, uint32_t height) throw(SDLException)
  : width(width), height(height) {
  // create SDL surface
  screen = create_screen(width, height);
  flipcnt = 0;
}

/// create a display window
SDL_Surface *SDLYuvViewer::create_screen(uint32_t width, uint32_t height) throw(SDLException) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    throw SDLException(SDL_GetError());
  }

  // set the @exit hook for SDL
  //atexit(SDL_Quit);
  //TODO(bfarkas): include method of properly quitting SDL on exit?

  // grab a surface on the screen
  return SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE | SDL_ANYFORMAT);
}

void SDLYuvViewer::setPixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b) throw(SDLException) {
  uint8_t *ubuff8;
  uint32_t *ubuff32;
  uint32_t color;
  char c1, c2, c3;

  // Lock the screen, if needed
  if (SDL_MUSTLOCK(screen)) {
    if (SDL_LockSurface(screen) < 0) {
      return;
    }
  }

  // Get the color
  color = SDL_MapRGB(screen->format, r, g, b);

  // how we draw the pixel depends on the bitdepth
  switch (screen->format->BytesPerPixel) {
    case 3:
      ubuff8   = reinterpret_cast<uint8_t *>(screen->pixels);
      ubuff8  += (y * screen->pitch) + (x * 3);

      if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
        c1 = (color & 0xFF0000) >> 16;
        c2 = (color & 0x00FF00) >> 8;
        c3 = (color & 0x0000FF);
      } else {
        c3 = (color & 0xFF0000) >> 16;
        c2 = (color & 0x00FF00) >> 8;
        c1 = (color & 0x0000FF);
      }
      ubuff8[0] = c3;
      ubuff8[1] = c2;
      ubuff8[2] = c1;
      break;

    case 4:
      ubuff8   = reinterpret_cast<uint8_t *>(screen->pixels);
      ubuff8  += (y * screen->pitch) + (x * 4);
      ubuff32  = reinterpret_cast<uint32_t *>(ubuff8);
      *ubuff32 = color;
      break;

    default:
      throw SDLException(const_cast<char *>("Error: Unknown bitdepth!"));
  }

  // unlock the screen if needed
  if (SDL_MUSTLOCK(screen)) {
    SDL_UnlockSurface(screen);
  }
}

/// Convert YUV 4:2:2 to RGB values.
/// The formulas are taken from http://www.fourcc.org.
/// @param yuv YUVQuad object.
/// @param rgb1 RGBQUAD struct to store the data of the first RGB pixel in.
/// @param rgb2 RGBQUAD struct to store the data of the second RGB pixel in.
inline void SDLYuvViewer::yuv422_to_rgb(uint8_t *yuv, RGBQUAD &rgb1, RGBQUAD &rgb2) {
  float r, g, b;

  // first RGBQUAD
  b = 1.164 * static_cast<float>(yuv[1] - 16) + 2.018 * static_cast<float>(yuv[0] - 128);
  g = 1.164 * static_cast<float>(yuv[1] - 16) - 0.813 * static_cast<float>(yuv[2] - 128)
                                              - 0.391 * static_cast<float>(yuv[0] - 128);
  r = 1.164 * static_cast<float>(yuv[1] - 16) + 1.596 * static_cast<float>(yuv[2] - 128);
  if (r < 0) {
    r = 0;
  }
  if (r > 255) {
    r = 255;
  }

  if (g < 0) {
    g = 0;
  }
  if (g > 255) {
    g = 255;
  }

  if (b < 0) {
    b = 0;
  }
  if (b > 255) {
    b = 255;
  }

  rgb1.rgbRed = (uint32_t)r;
  rgb1.rgbGreen = (uint32_t)g;
  rgb1.rgbBlue = (uint32_t)b;

  // second RGBQUAD
  b = 1.164 * static_cast<float>(yuv[3] - 16) + 2.018 * static_cast<float>(yuv[0] - 128);
  g = 1.164 * static_cast<float>(yuv[3] - 16) - 0.813 * static_cast<float>(yuv[2] - 128)
                                              - 0.391 * static_cast<float>(yuv[0] - 128);
  r = 1.164 * static_cast<float>(yuv[3] - 16) + 1.596 * static_cast<float>(yuv[2] - 128);
  if (r < 0) {
    r = 0;
  }
  if (r > 255) {
    r = 255;
  }

  if (g < 0) {
    g = 0;
  }
  if (g > 255) {
    g = 255;
  }

  if (b < 0) {
    b = 0;
  }
  if (b > 255) {
    b = 255;
  }

  rgb2.rgbRed = (uint32_t)r;
  rgb2.rgbGreen = (uint32_t)g;
  rgb2.rgbBlue = (uint32_t)b;
}

void SDLYuvViewer::drawYUVVector(uint8_t *yuvframe, uint32_t x, uint32_t y) {
  uint32_t xpos, ind;
  RGBQUAD rgb1, rgb2;

  for (xpos = 0; xpos < width * 2; xpos += 4) {
    ind = xpos;
    // convert yuv to rgb
    yuv422_to_rgb(&yuvframe[ind], rgb1, rgb2);

    setPixel(x + xpos / 2, y, rgb1.rgbRed, rgb1.rgbGreen, rgb1.rgbBlue);
    setPixel(x + xpos / 2 + 1, y, rgb2.rgbRed, rgb2.rgbGreen, rgb2.rgbBlue);
  }

  if (y == height - 1) {
    SDL_Flip(screen);
  }
}

void SDLYuvViewer::drawRect(uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height,
  uint32_t thickness,
  uint32_t r,
  uint32_t g,
  uint32_t b) {
  uint32_t t;

  if ((width == 0) || (height == 0)) {
    /* no valid target found */
    return;
  }

  // horizontal lines
  for (uint32_t xpos = x; xpos < x + width; xpos++) {
    for (t = 0; t < thickness; t++) {
      setPixel(xpos + t, y + t,        r, g, b);
      setPixel(xpos - t, y - t + height, r, g, b);
    }
  }

  // vertical lines
  for (uint32_t ypos = y; ypos < y + height; ypos++) {
    for (t = 0; t < thickness; t++) {
      setPixel(x + t,       ypos, r, g, b);
      setPixel(x - t + width, ypos, r, g, b);
    }
  }
  SDL_Flip(screen);
}

char SDLYuvViewer::check_for_input() {
  SDL_Event event;
  char key = 0;
  while(SDL_PollEvent(&event)){
    if (event.type == SDL_QUIT /*|| (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) )*/) {
      sc_core::sc_stop();
    }
    else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            sc_core::sc_stop(); 
            break;
        case SDLK_q:
            sc_core::sc_stop();
            break;
        case SDLK_LEFT:
            key = 'l';
            break;
        case SDLK_RIGHT:
            key = 'r';
            break;
        case SDLK_UP:
            key = 'u';
            break;
        case SDLK_DOWN:
            key = 'd';
            break;
        default:
            break;
      }
    }
    else if (event.type == SDL_KEYUP){
      key = 1;
    }
    else {
      break;
    }
  }
  return key;
}

void SDLYuvViewer::quit() {
  SDL_Quit();
}

SDLYuvViewer::~SDLYuvViewer() {
}
/// @}
