// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdemosoftware AHB Demonstration Input Device
/// @{
/// @file ahbdemosoftware.h
/// Class definition for input device. Provides frames of data samples in regual
/// intervals. CPU is notifed about new data via IRQ.
///
/// @date 2010-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Thomas Schuster
///

#ifndef MODELS_AHBDEMOSOFTWARE_AHBDEMOSOFTWARE_H_
#define MODELS_AHBDEMOSOFTWARE_AHBDEMOSOFTWARE_H_

// The TLM header (to be included in each TL model)
#include <tlm.h>

// Provides methods for generating random data
#include <math.h>

// AHB TLM master socket and protocol implementation
#include "core/common/ahbmaster.h"
// Timing interface (specify clock period)
#include "core/common/clkdevice.h"

// Verbosity kit - for output formatting and filtering
#include "core/common/verbose.h"

/// Definition of class AHBDemoSoftware
class AHBDemoSoftware : public AHBMaster<>, public CLKDevice {
  public:
    SC_HAS_PROCESS(AHBDemoSoftware);
    SR_HAS_SIGNALS(AHBDemoSoftware);

    sc_in<char> keyboardIn;
    sc_in<bool> triggerIn;
    sc_out<bool> triggerOut;

    /// Constructor
    AHBDemoSoftware(ModuleName name,  // The SystemC name of the component
    unsigned int hindex,                    // The master index for registering with the AHB
    bool pow_mon,                               // Enable power monitoring
    uint32_t frameWidth,
    uint32_t frameHeight,
    AbstractionLayer ambaLayer);            // TLM abstraction layer

    /// Thread for reading keyboard events
    void readKeyboard();

    void frameTrigger();

    /// Thread for generating the data frame
    void software();

    /// Reset function
    void dorst();

    /// Deal with clock changes
    void clkcng();

    sc_core::sc_time get_clock();

  protected:
    // display size and zoom window size and location
    uint32_t m_frameWidth;
    uint32_t m_frameHeight;
    
    char m_key;
    sc_event frameTriggerEvent;
  private:
    // data members
    // ------------

    /// ID of the AHB master
    const uint32_t m_master_id;

    /// Power monitoring enabled/disable
    bool m_pow_mon;

    /// amba abstraction layer
    AbstractionLayer m_abstractionLayer;

    void zoom(uint32_t mem, uint32_t in_x, uint32_t in_y, uint32_t out_x, uint32_t out_y, uint32_t video_width, uint32_t frame_width, uint32_t frame_height);
    void histogram(uint32_t mem, uint32_t in_x, uint32_t in_y, uint32_t out_x, uint32_t out_y, uint32_t video_width, uint32_t frame_width, uint32_t frame_height);
};

#endif  // MODELS_AHBDEMOSOFTWARE_AHBDEMOSOFTWARE_H_
/// @}
