// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdisplay
/// @{
/// @file yuv_viewer.h
///
///
/// @date 2013-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author
///
#ifndef MODELS_AHBDISPLAY_YUV_VIEWER_H_
#define MODELS_AHBDISPLAY_YUV_VIEWER_H_

#include <SDL/SDL.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>

#define YUV_VIEWER_DEFAULT_WIDTH  1024
#define YUV_VIEWER_DEFAULT_HEIGHT 768

typedef struct tagRGBQUAD {
  uint32_t rgbBlue;
  uint32_t rgbGreen;
  uint32_t rgbRed;
  uint32_t rgbReserved;
} RGBQUAD;

class SDLException {
  public:
    char reason[256];
    explicit SDLException(char *msg) throw() {
      snprintf(reason, strlen(msg), "%s",msg);
    }
    const char*what() throw() {
      return reason;
    }
    ~SDLException() throw() {}
};

/// This class creates a window with a graphical surface and
/// provides methods for drawing YUVFrames and BINFrames.
/// Furthermore, some simple drawing functions are available.
///
/// This implementation uses the SDL library (Simple Direct Media Layer)
/// and thus runs under Linux, Windows, Solaris, and all other operating
/// systems which are supported by the SDL.
class SDLYuvViewer {
  public:
    /// Create a new yuv_viewer window with default width and height.
    /// @throw SDLException if an error occured.
    SDLYuvViewer() throw(SDLException);

    /// Constructor, which creates a new YUV viewer
    /// window with custom width and height.
    /// @param width The width of the new window
    /// @param height The height of the new window
    /// @throw SDLException if an error occured.
    SDLYuvViewer(uint32_t width, uint32_t height) throw(SDLException);

    /// Close this window (called by delete).
    ~SDLYuvViewer();

    /// Draw a YUVFrame at the specified position.
    /// @param yuvframe A pointer to the YUVFrame object.
    /// @param x Top left corner x coordinate
    /// @param y Top left corner y coordinate
    void drawYUVVector(uint8_t *yuvframe, uint32_t x, uint32_t y);

    /// Paints a color rectangle.
    /// @param x Top left corner x coordinate
    /// @param y Top left corner y coordinate
    /// @param width Width of the rectangle
    /// @param height Height of the rectangle
    /// @param thickness Border thickness in pixel
    /// @param r Red color value
    /// @param g Green color value
    /// @param b Blue color value
    void drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    uint32_t thickness, uint32_t r, uint32_t g, uint32_t b);

    /// Paint a pixel of the given color. This method shows no
    /// effect until you call the SDL_Flip function, which
    /// updates the screen.
    ///
    /// @param x Pixel x
    /// @param y Pixel y
    /// @param r Red color value
    /// @param g Green color value
    /// @param b Blue color value
    void setPixel(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b) throw(SDLException);

    /// Poll whether a quit request was detected.
    /// @return True if the user hit the [x] of the YUV viewer window
    ///         or pressed CTRL-c.
    // SDL_Event check_for_quit();

    /// Update the screen. This function flushes all recent
    /// changes to the image buffer onto the screen.
    void update() {}

    /// Poll keyboard events.
    char check_for_input();

    void quit();

    void yuv422_to_rgb(uint8_t *yuv, RGBQUAD &rgb1, RGBQUAD &rgb2);

  protected:
    // members
    uint32_t width, height;
    SDL_Surface *screen;
    uint32_t flipcnt;

    // functions
    SDL_Surface*create_screen(uint32_t width, uint32_t height) throw(SDLException);
};

#endif  // MODELS_AHBDISPLAY_YUV_VIEWER_H_
/// @}
