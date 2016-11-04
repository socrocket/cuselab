// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbgrayframer
/// @{
/// @file ahbgrayframer.cpp
///
///
/// @date 2013-2015
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Bastian Farkas
///
#include "models/ahbgrayframer/ahbgrayframer.h"
#include "core/common/verbose.h"

AHBGrayframer::AHBGrayframer(sc_module_name name,
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
  AbstractionLayer ambaLayer) :
  AHBMaster<APBSlave>(
    name,
    hindex,
    0x03,
    0x005,
    0,
    0,
    ambaLayer,
    BAR(), BAR(), BAR(), BAR()),
  m_videoaddr(0xA0000000),
  m_video_width(video_width),
  m_frame_width(frame_width), m_frame_height(frame_height),
  m_in_x(in_x), m_in_y(in_y),
  m_out_x(out_x), m_out_y(out_y),
  m_factor(2), 
  m_buffer(NULL), m_frameToggle(true),
  m_channel(channel),
  m_grayframer_initialised(false) {
  init_apb(pindex, 0x03, 0x005, 0, 0, APBIO, pmask, 0, 0, paddr);

  init_registers();
  // Die Threads der Klasse
  SC_THREAD(paint_it_gray);
  SC_METHOD(frameTrigger);
  sensitive << triggerIn;
}

void AHBGrayframer::init_registers() {
  r.create_register("CTRL", "Grayframer Control Register", 
    0x00,        // offset
    0x00,
    0xFF)
  .callback(SR_PRE_READ, this, &AHBGrayframer::ctrl_read)
  .callback(SR_POST_WRITE, this, &AHBGrayframer::ctrl_write);
  r.create_register("ADDR", "Grayframer Video Address Register", 
    0x04,  // offset
    m_videoaddr,
    0xFFFFF000);
  r.create_register("IN_POS", "Grayframer Input Register", 
    0x08,       // offset
    (m_in_x << 16) | m_in_y,
    0xFFFFFFFF);
  r.create_register("OUT_POS", "Grayframer Output Register", 
    0x0C,     // offset
    (m_out_x << 16) | m_out_y,
    0xFFFFFFFF);
  r.create_register("SIZE", "Grayframer Frame Size Register", 
    0x10,     // offset
    (m_frame_width << 16) | m_frame_height,
    0xFFFFFFFF);
}

AHBGrayframer::~AHBGrayframer() {
  GC_UNREGISTER_CALLBACKS();
}

void AHBGrayframer::end_of_elaboration() {
}

void AHBGrayframer::ctrl_read() {
  uint32_t reg = 0;
  reg |= (m_buffer) ? 1 : 0 << 0;
  reg |= m_frameToggle << 1;
  r[0x0] = reg;
}

void AHBGrayframer::ctrl_write() {
  if (r[0x0] & 0x2) {
    frameTriggerEvent.notify();
  }
  if ((r[0x0] & 0x1) && !m_grayframer_initialised) {
    if (m_buffer) {
      delete[] m_buffer;
    }
    init_grayframer();
  }
  if (!(r[0x0 & 0x1]) && m_grayframer_initialised) {
  }
}

void AHBGrayframer::init_grayframer() {
  m_videoaddr = r[0x4];
  //m_video_width = ((r[0x10] >> 16) & 0xFFFF)/2; //320 //TODO: need new register vor video size!!!
  m_frame_width = (r[0x10] >> 16) & 0xFFFF; //640
  m_frame_height = ((r[0x10] >>  0) & 0xFFFF); //480
  m_in_x = (r[0x8] >> 16) & 0xFFFF; //0
  m_in_y = (r[0x8] >>  0) & 0xFFFF; //0
  m_out_x = (r[0xC] >> 16) & 0xFFFF; //320
  m_out_y = (r[0xC] >>  0) & 0xFFFF; //0
  m_factor = m_frame_width / m_video_width;
  m_buffer = new uint8_t[m_video_width * 2];
  m_grayframer_initialised = true;

  v::info << name() << "CTRL    r[0x00]: " << v::uint32 << (uint32_t)r[0x0] << v::endl;
  v::info << name() << "ADDR    r[0x04]: " << v::uint32 << (uint32_t)r[0x4] << v::endl;
  v::info << name() << "IN_POS  r[0x08]: " << v::uint32 << (uint32_t)r[0x8] << v::endl;
  v::info << name() << "OUT_POS r[0x0C]: " << v::uint32 << (uint32_t)r[0xC] << v::endl;
  v::info << name() << "SIZE    r[0x10]: " << v::uint32 << (uint32_t)r[0x10] << v::endl;
}

void AHBGrayframer::frameTrigger(){
  frameTriggerEvent.notify();
}

// this thread reads a row every 18 us so it take 13.824 ms to read a whole picture
// together with the porches and blanking its 14.508 ms which equals about 69 Hz frame rate
// (which is in fact what we have in reality...)
void AHBGrayframer::paint_it_gray() {
  m_frameToggle = false;
  uint32_t i,x;
  while (true) {
    wait(frameTriggerEvent);
    
    for (i = 0; i < m_frame_height/m_factor; i++) {
          ahbread(m_videoaddr + m_in_x * 2+ m_in_y * m_video_width * 2 * m_factor + i * m_video_width * 2 * m_factor,
            m_buffer,
            m_video_width*2);

          switch(m_channel){
            case 'Y':
              for (x = 0; x < m_video_width*2; x+=4) {
                m_buffer[x+0] = 128;
                m_buffer[x+2] = 128;
              }
              break;
            case 'U':
              for (x = 0; x < m_video_width*2; x+=4) {
                m_buffer[x+1] = 128;
                m_buffer[x+2] = 128;
                m_buffer[x+3] = 128;
              }
              break;
            case 'V':
              for (x = 0; x < m_video_width*2; x+=4) {
                m_buffer[x+0] = 128;
                m_buffer[x+1] = 128;
                m_buffer[x+3] = 128;
              }
              break;
            default:
              break;
          } 

          ahbwrite(m_videoaddr + m_out_x * 2 + m_out_y * m_video_width * 2 * m_factor + i * m_video_width * 2 * m_factor,
            m_buffer,
            m_video_width*2);
          //wait(20 * clock_cycle);
    }
    // wait(400*clock_cycle); //wait front porch, back porch and blanking time
    m_frameToggle = !m_frameToggle;
    triggerOut.write(m_frameToggle);
  }
}

/// @}
