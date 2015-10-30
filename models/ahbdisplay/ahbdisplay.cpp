// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdisplay
/// @{
/// @file ahbdisplay.cpp
///
///
/// @date 2013-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Rolf Meyer
///
#include "models/ahbdisplay/ahbdisplay.h"
#include "core/common/verbose.h"
#include "core/common/sr_report.h"
#include "core/common/sr_registry.h"

AHBDisplay::AHBDisplay(ModuleName name,
  uint32_t hindex,
  uint32_t pindex,
  uint32_t paddr,
  uint32_t pmask,
  uint32_t frame_width,
  uint32_t frame_height,
  AbstractionLayer ambaLayer) :
  AHBMaster<APBSlave>(name,
    hindex,
    0x03,
    0x003,
    0,
    0,
    ambaLayer/*,
    BAR(), BAR(), BAR(), BAR()*/),
  m_screen(NULL),
  m_videoaddr(0xA0000000),
  m_width(frame_width),
  m_height(frame_height),
  m_rowDuration(ROW_DURATION_IN_NS, SC_NS) {
  m_xferData = new uint8_t[m_width * 4];
  init_apb(pindex, 0x03, 0x003, 0, 0, APBIO, pmask, 0, 0, paddr);

  init_registers();
  // Die Threads der Klasse
  SC_THREAD(yf_painter);
  SC_METHOD(frameTrigger);
  sensitive << triggerIn;

}

void AHBDisplay::init_registers(){
  r.create_register("CTRL", "Display Control Register", 0x00,        // offset
    0x00,
    0xFF)
  .callback(SR_PRE_READ, this, &AHBDisplay::ctrl_read)
  .callback(SR_POST_WRITE, this, &AHBDisplay::ctrl_write);
  r.create_register("ADDR", "Display Video Address Register", 0x04,  // offset
    m_videoaddr,
    0xFFFFF000);
  r.create_register("Width", "Display Width Register", 0x08,         // offset
    m_width,
    0xFFFFFFFF);
  r.create_register("Height", "Display Height Register", 0x0C,       // offset
    m_height,
    0xFFFFFFFF);
}

AHBDisplay::~AHBDisplay() {
  if (m_screen) {
    delete m_screen;
  }
  GC_UNREGISTER_CALLBACKS();
}

void AHBDisplay::end_of_elaboration() {
}

void AHBDisplay::end_of_simulation() {
  m_screen->quit();
}

void AHBDisplay::ctrl_read() {
  uint32_t reg = 0;
  reg = ((m_screen != NULL) & 0x1) << 0;
  r[0x0] = reg;
}

void AHBDisplay::ctrl_write() {
  if ((r[0x0] & 0x1) && (m_screen == NULL)) {
    init_yuv_viewer();
  }
  if ((!(r[0x0] & 0x1)) && m_screen) {
    delete m_screen;
    m_screen = NULL;
  }
  if (r[0x0] & 0x2) {
    frameTriggerEvent.notify();
  }
}

void AHBDisplay::frameTrigger(){
  frameTriggerEvent.notify();
}

bool AHBDisplay::init_yuv_viewer() {
  m_videoaddr = r[0x4];
  m_width = r[0x8];
  m_height = r[0xC];
  v::info << name() << "Open Screen with width " << m_width << " and height " << m_height << v::endl;
  m_screen = new SDLYuvViewer(m_width, m_height);
 
  v::info << name() << "CTRL   r[0x00]: " << v::uint32 << (uint32_t)r[0x0] << v::endl;
  v::info << name() << "ADDR   r[0x04]: " << v::uint32 << (uint32_t)r[0x4] << v::endl;
  v::info << name() << "Width  r[0x08]: " << v::uint32 << (uint32_t)r[0x8] << v::endl;
  v::info << name() << "Height r[0x0C]: " << v::uint32 << (uint32_t)r[0xC] << v::endl;
  
  return m_screen != 0;
}

// this thread reads a row every 18 us so it take 13.824 ms to read a whole picture
// together with the porches and blanking its 14.508 ms which equals about 69 Hz frame rate
// (which is in fact what we have in reality...)
//const uint32_t memoffset = 0x40000000;

void AHBDisplay::yf_painter() {
  char key;
  while (true) {
    wait(frameTriggerEvent);
    //v::info << name() << "Paint screen" << v::endl;

    for (uint32_t i = 0; i < m_height; i++) {
        ahbread(m_videoaddr + (i * m_width * 2), m_xferData, m_width * 2);
        m_screen->drawYUVVector(m_xferData, 0, i);

        wait(1 * clock_cycle);
    }
    key = m_screen->check_for_input();
    if (key) {
      keyboardOut.write(key);
    }
    // wait(PORCH_AND_BLANK_DURATION_IN_NS,SC_NS); //wait front porch, back porch and blanking time
  }
}

/// @}
