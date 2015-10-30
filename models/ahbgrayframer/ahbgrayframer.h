// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbgrayframer
/// @{
/// @file ahbgrayframer.h
///
///
/// @date 2013-2015
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Bastian Farkas
///
#ifndef MODELS_AHBGRAYFRAMER_AHBGRAYFRAMER_H_
#define MODELS_AHBGRAYFRAMER_AHBGRAYFRAMER_H_

#include <amba.h>
//#include <greenreg_ambasockets.h>

#include "core/common/base.h"
#include "core/common/ahbmaster.h"
#include "core/common/apbdevice.h"
#include "core/common/apbslave.h"
#include "core/common/clkdevice.h"

#include "core/common/sr_signal.h"

class AHBGrayframer : public AHBMaster<APBSlave>, public CLKDevice {
  public:
    SC_HAS_PROCESS(AHBGrayframer);
    SR_HAS_SIGNALS(AHBGrayframer);
    GC_HAS_CALLBACKS();

    sc_in<bool> triggerIn;
    sc_out<bool> triggerOut;

    AHBGrayframer(sc_module_name name,
    uint32_t hindex,
    uint32_t pindex,
    uint32_t paddr,
    uint32_t pmask,
    char channel,
    uint32_t in_x,
    uint32_t in_y,
    uint32_t out_x,
    uint32_t out_y,
    uint32_t video_width,
    uint32_t frame_width,
    uint32_t frame_height,
    AbstractionLayer ambaLayer = amba::amba_LT);

    /// Destructor
    ~AHBGrayframer();

    void init_registers();
    void end_of_elaboration();

    sc_event frameDone;
    sc_event frameTriggerEvent;

    sc_core::sc_time get_clock() {return clock_cycle; }

    uint8_t *memory;

  protected:
    void init_grayframer();
    void paint_it_gray();
    void frameTrigger();

    void ctrl_read();
    void ctrl_write();

    uint32_t m_videoaddr;
    uint32_t m_video_width;
    uint32_t m_frame_width;
    uint32_t m_frame_height;
    uint32_t m_in_x;
    uint32_t m_in_y;
    uint32_t m_out_x;
    uint32_t m_out_y;
    uint32_t m_factor;

    uint8_t *m_buffer;
    bool m_frameToggle;
    char m_channel;
    bool m_grayframer_initialised;
    sc_time *delay;
};

#endif  // MODELS_AHBGRAYFRAMER_AHBGRAYFRAMER_H_
/// @}
