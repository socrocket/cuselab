// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdisplay AHBDisplay
/// @{
/// @file ahbdisplay.h
///
///
/// @date 2013-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Rolf Meyer
///
#ifndef MODELS_AHBDISPLAY_AHBDISPLAY_H_
#define MODELS_AHBDISPLAY_AHBDISPLAY_H_

#define ROW_DURATION_IN_NS 18000
#define PORCH_AND_BLANK_DURATION_IN_NS 684000
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

//#include <amba.h>
//#include <greenreg_ambasockets.h>
#include "core/common/amba.h"
#include "core/common/base.h"
#include "core/common/systemc.h"
#include <boost/config.hpp>

#include <SDL/SDL.h>

#include "models/ahbdisplay/yuv_viewer.h"

#include "core/common/ahbmaster.h"
#include "core/common/apbdevice.h"
#include "core/common/apbslave.h"
#include "core/common/clkdevice.h"

#include "core/common/sr_signal.h"

/**
 * Open a YUV-Viewer window, receive YUVFrames from
 * a connected ship_channel, and paint them onto the screen.
 */
class AHBDisplay : public AHBMaster<APBSlave>, public CLKDevice {
  public:
    SC_HAS_PROCESS(AHBDisplay);
    SR_HAS_SIGNALS(AHBDisplay);
    GC_HAS_CALLBACKS();

    uint32_t yuvcount;

    sc_in<bool> triggerIn;
    sc_out<char> keyboardOut;
    signal<std::pair<uint32_t, bool> >::out irq;

    AHBDisplay(ModuleName name, 
    uint32_t hindex, 
    uint32_t pindex, 
    uint32_t paddr, 
    uint32_t pmask, 
    uint32_t frame_width,
    uint32_t frame_height,
    AbstractionLayer ambaLayer = amba::amba_LT);

    ~AHBDisplay();

    void init_registers();
    void end_of_elaboration();
    void end_of_simulation();

    sc_event frameDone;
    sc_event frameTriggerEvent;
    void frameTrigger();

    sc_core::sc_time get_clock() {return clock_cycle; }
    uint8_t *memory;

  protected:
    /// Initialize the X11 window
    bool init_yuv_viewer();

    /// SC_THREAD to receive and paint yuvframes
    void yf_painter();

    void ctrl_read();
    void ctrl_write();

    /// Reference to our X11 window
    SDLYuvViewer *m_screen;

    SDL_Event event;
    uint32_t m_videoaddr;
    uint32_t m_width;
    uint32_t m_height;

    sc_time m_rowDuration;
    uint8_t *m_xferData;
    sc_time *delay;
};

#endif  // MODELS_AHBDISPLAY_AHBDISPLAY_H_
/// @}
