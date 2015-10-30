// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup apbkeyboard
/// @{
/// @file apbkeyboard.cpp
///
///
/// @date 2010-2015
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Bastian Farkas
///

#include "models/apbkeyboard/apbkeyboard.h"
#include "core/common/sr_registry.h"
#include <string>

SR_HAS_MODULE(APBKeyboard);

// Constructor: create all members, registers and Counter objects.
// Store configuration default value in conf_defaults.
APBKeyboard::APBKeyboard(ModuleName name,
  uint16_t pindex,
  uint16_t paddr,
  uint16_t pmask,
  uint16_t pirq) :
  APBSlave(name, pindex, 0x1, 0x00C, 1, pirq, APBIO, pmask, false, false, paddr){
  SC_THREAD(update_key);
  SC_METHOD(get_key);
  sensitive << keyboardIn;

  init_registers();
}

// Destructor: Unregister Register Callbacks.
// Destroy all Counter objects.
APBKeyboard::~APBKeyboard() {
  GC_UNREGISTER_CALLBACKS();
}

void APBKeyboard::init_registers() {
  /* create register */
  r.create_register("data", "Keyboard Data Register",
    0,        // offset
    0x00,     // init value
    0xFF);    // write mask
}

void APBKeyboard::get_key() {
  lastkey = keyboardIn.read();
  r[0x0] = lastkey;
  keyReceived.notify();
}

void APBKeyboard::update_key() {
  while(1) {
    wait(keyReceived);
    v::info << name() << "got key: " << r[0x0] << v::endl;
  }
}
