// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbframetrigger
/// @{
/// @file ahbframetrigger.cpp
/// Class definition for AHBFrameTrigger. Provides frames of data samples in regual
/// intervals. CPU is notifed about new data via IRQ.
///
/// @date 2010-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Thomas Schuster
///

#include "models/ahbframetrigger/ahbframetrigger.h"

/// Constructor
AHBFrameTrigger::AHBFrameTrigger(ModuleName name,     // The SystemC name of the component
  unsigned int hindex,                         // The master index for registering with the AHB
  sc_core::sc_time interval,                   // The interval between data frames
  bool pow_mon,                                // Enable power monitoring
  AbstractionLayer ambaLayer) :            // TLM abstraction layer
    AHBMaster<>(name,                            // SystemC name
      hindex,                                    // Bus master index
      0x04,                                      // Vender ID (4 = ESA)
      0x00,                                      // Device ID (undefined)
      0,                                         // Version
      0,                                         // IRQ of device
      ambaLayer),                                // AmbaLayer
    m_interval(interval),                        // Initialize frame interval
    m_master_id(hindex),                         // Initialize bus index
    m_pow_mon(pow_mon),                          // Initialize pow_mon
    m_abstractionLayer(ambaLayer) {              // Initialize abstraction layer
  // Register frame_trigger thread
  SC_THREAD(frame_trigger);

  // Register thread for generating the actual data frame
  SC_THREAD(gen_frame);
  sensitive << new_frame;

  // Module Configuration Report
  v::info << this->name() << " ************************************************** " << v::endl;
  v::info << this->name() << " * Created AHBFrameTrigger in following configuration: " << v::endl;
  v::info << this->name() << " * --------------------------------------------- " << v::endl;
  v::info << this->name() << " * abstraction Layer (LT = 8 / AT = 4): " << m_abstractionLayer << v::endl;
  v::info << this->name() << " ************************************************** " << v::endl;
}

// Rest handler
void AHBFrameTrigger::dorst() {
  // Nothing to do
}

// This thread generates a "new_frame" event at intervals "m_interval"
void AHBFrameTrigger::frame_trigger() {
  while (1) {
    // Wait for m_interval SystemC time
    wait(m_interval);

    // Activate "new_frame" event
    new_frame.notify();
  }
}

// Generates a thread of frame_length
void AHBFrameTrigger::gen_frame() {
  // Locals
  uint32_t data;
  // uint32_t debug;
  // sc_core::sc_time delay = SC_ZERO_TIME;

  // Wait for system becoming ready
  wait(1, SC_MS);

  data = 0x03000000; // Endianess! 
  ahbwrite(0x80050000,(uint8_t*)&data,4); // ahbdisplay
  wait(1, SC_MS);
  data = 0x03000000; // Endianess! 
  ahbwrite(0x80050200,(uint8_t*)&data,4); // ahbgrayframer
  wait(1, SC_MS);
  while (1) {
    data = 0x03004001; // Endianess! 
    ahbwrite(0x80050100,(uint8_t*)&data,4); // ahbcamera
    wait(40, SC_MS);
  }
}

// Helper for setting clock cycle latency using a value-time_unit pair
void AHBFrameTrigger::clkcng() {
  // nothing to do
}

sc_core::sc_time AHBFrameTrigger::get_clock() {
  return clock_cycle;
}

/// @}
