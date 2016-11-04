// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup apbkeyboard
/// @{
/// @file apbkeyboard.h
///
///
/// @date 2010-2015
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Bastian Farkas
///

#ifndef MODELS_APBKEYBOARD_APBKEYBOARD_H_
#include "core/common/systemc.h"
#include "core/common/apbdevice.h"
#include "core/common/clkdevice.h"
#include "core/common/sr_signal.h"
#include "core/common/verbose.h"
#include "core/common/apbslave.h"

/// @brief This class is a TLM 2.0 Model of a very basic Keyboard.
class APBKeyboard : public APBSlave, public CLKDevice {
  public:
    SC_HAS_PROCESS(APBKeyboard);
    SR_HAS_SIGNALS(APBKeyboard);
    GC_HAS_CALLBACKS();

    sc_in<char> keyboardIn;
    sc_event keyReceived;

    APBKeyboard(ModuleName name, uint16_t pindex = 0,
    uint16_t paddr = 0, uint16_t pmask = 4095, uint16_t pirq = 0
    );

    /// Free all counter and unregister all callbacks.
    ~APBKeyboard();
    void init_registers();

    // Register Callbacks
    //TODO needed?

    // SCTHREADS
    void update_key();
    // SCMETHODS
    void get_key();

    char lastkey;
};

#endif  // MODELS_APBUART_APBUART_H_
/// @}
