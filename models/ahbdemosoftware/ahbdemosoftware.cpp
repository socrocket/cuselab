// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup ahbdemosoftware
/// @{
/// @file ahbdemosoftware.cpp
/// Class definition for AHBDemoSoftware. Provides frames of data samples in regual
/// intervals. CPU is notifed about new data via IRQ.
///
/// @date 2010-2014
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the
///            authors is strictly prohibited.
/// @author Thomas Schuster
///

#include "models/ahbdemosoftware/ahbdemosoftware.h"

/// Constructor
AHBDemoSoftware::AHBDemoSoftware(ModuleName name,     // The SystemC name of the component
  unsigned int hindex,                         // The master index for registering with the AHB
  bool pow_mon,                                // Enable power monitoring
  uint32_t frameWidth,
  uint32_t frameHeight,
  AbstractionLayer ambaLayer) :            // TLM abstraction layer
    AHBMaster<>(name,                            // SystemC name
      hindex,                                    // Bus master index
      0x04,                                      // Vender ID (4 = ESA)
      0x00,                                      // Device ID (undefined)
      0,                                         // Version
      0,                                         // IRQ of device
      ambaLayer),                                // AmbaLayer
    m_frameWidth(frameWidth),
    m_frameHeight(frameHeight),
    m_master_id(hindex),                         // Initialize bus index
    m_pow_mon(pow_mon),                          // Initialize pow_mon
    m_abstractionLayer(ambaLayer) {              // Initialize abstraction layer
  // Register frame_trigger thread
  SC_THREAD(software);

  SC_METHOD(readKeyboard);
  sensitive << keyboardIn;

  SC_METHOD(frameTrigger);
  sensitive << triggerIn;

  // Module Configuration Report
  v::info << this->name() << "Created AHBDemoSoftware" << v::endl;
}

// Rest handler
void AHBDemoSoftware::dorst() {
  // Nothing to do
}

void AHBDemoSoftware::readKeyboard() {
  m_key = keyboardIn.read();
}

void AHBDemoSoftware::frameTrigger() {
  frameTriggerEvent.notify();
}

void AHBDemoSoftware::zoom(uint32_t mem, uint32_t in_x, uint32_t in_y, uint32_t out_x, uint32_t out_y, uint32_t video_width, uint32_t frame_width, uint32_t frame_height) {
    int i, j;
    uint8_t factor = frame_width / video_width;
    uint8_t inbuffer[video_width];
    uint8_t outbuffer[video_width*2];
    for(i=0;i<frame_height/(factor*2);i++){
          ahbread(
              mem + (in_y*video_width*2*factor) + in_x * 2 + (i * video_width * 2 * factor ),
              inbuffer, 
              video_width
          );
          // double pixels within one line
          for(j=0;j<video_width;j+=4) { // only take y1 and y2 values?
            outbuffer[   j *2] = inbuffer[j];
            outbuffer[1+ j *2] = inbuffer[j+1];
            outbuffer[2+ j *2] = inbuffer[j+2];
            outbuffer[3+ j *2] = inbuffer[j+1];
            outbuffer[4+ j *2] = inbuffer[j];
            outbuffer[5+ j *2] = inbuffer[j+3];
            outbuffer[6+ j *2] = inbuffer[j+2];
            outbuffer[7+ j *2] = inbuffer[j+3];
          }
          // write line 2 times
          ahbwrite(
              mem + (out_y*video_width*2*factor) + out_x * 2 + (i * video_width * 4*factor ), 
              outbuffer, 
              video_width*2
          );
          ahbwrite(
              mem + ((out_y+1)*video_width*2*factor) + out_x * 2 + (i * video_width * 4*factor ), 
              outbuffer, 
              video_width*2
          );
    }
}

void AHBDemoSoftware::histogram(uint32_t mem, uint32_t in_x, uint32_t in_y, uint32_t out_x, uint32_t out_y, uint32_t video_width, uint32_t frame_width, uint32_t frame_height) {
    int i, j, tmp;
    uint8_t inbuffer[video_width];
    uint8_t outbuffer[64*4];
    int histogramdata[64];
    uint8_t histheight;
    uint8_t factor = frame_width / video_width;
    memset(histogramdata,0,64*4);
    // read data for histogram line by line and store in 64 buckets, max height should be 192, actual max height would be 19200
    for(i=0;i<frame_height/(2*factor);i++){
          ahbread(
              mem+(in_y*video_width*2*factor) + in_x * 2 + (i * video_width * 2*factor ),
              inbuffer, 
              video_width
          );
          // go through line and count greyvalues
          for(j=0;j<video_width;j+=4){
            histogramdata[(inbuffer[j+1])/4]++;
            histogramdata[(inbuffer[j+3])/4]++;
          }
    }
    // create histogram in full frame
    /*for(i=0;i<64;i++) {
      tmp = histogramdata[i];
      printf("%d,",tmp);
    }
    printf("\n");*/

    
    // draw histogram from full frame one line at a time
    histheight = 192;
    // max in histdata / histheigt = scalefactor
    uint32_t histmax = histogramdata[0];
    for (i=0;i<64;i++) {
        if(histogramdata[i]>histmax) {
          histmax = histogramdata[i];
        }
    }
    uint8_t histscalefactor = histmax/histheight;
    for(i=0;i<histheight;i++){
        for(j=0;j<64*4;j+=4) {
          if ((histogramdata[j/4]/histscalefactor) >= ((histheight)-i)) {  
            outbuffer[j+1] = 0;
            outbuffer[j+3] = 0;
            outbuffer[j+0] = 128;
            outbuffer[j+2] = 128;
          }
          else {
            outbuffer[j+1] = 255;
            outbuffer[j+3] = 255;
            outbuffer[j+0] = 128;
            outbuffer[j+2] = 128;
          }
        }
          ahbwrite(
              mem+(out_y*video_width*2*factor) + out_x * 2 + (i * video_width * 2*factor ), 
              outbuffer, 
              64*4 
          );
    }
}

// Generates a thread of frame_length
void AHBDemoSoftware::software() {
  // Locals
  uint32_t i, inPosX = 0, inPosY = 0, outPosX = 0, outPosY = 240;
  uint32_t windowWidth = 160, windowHeight = 120;
  uint8_t *buffer = new uint8_t[windowWidth*2];
  uint32_t videoaddr = 0xA0000000;
  bool frameToggle = false;
  // Wait for system becoming ready
  wait(1, SC_MS);
  while(1) {
    wait(frameTriggerEvent);
    switch(m_key){
      case 'r':
        if (inPosX<160){
          inPosX+=2;
        }
        break;
      case 'l':
        if (inPosX>0)
          inPosX-=2;
        break;
      case 'u':
        if (inPosY>0) {
          inPosY-=2;
        }
        break;
      case 'd':
        if (inPosY<120) {
          inPosY+=2;
        }  
        break;
      default:
        break;
    }
    zoom(videoaddr, 320+inPosX, inPosY, 0, 240, 320, m_frameWidth, m_frameHeight);
    histogram(videoaddr, 320+inPosX, inPosY, 320, 240, 320, m_frameWidth, m_frameHeight);
    frameToggle = !frameToggle;
    triggerOut.write(frameToggle);
  }
}

// Helper for setting clock cycle latency using a value-time_unit pair
void AHBDemoSoftware::clkcng() {
  // nothing to do
}

sc_core::sc_time AHBDemoSoftware::get_clock() {
  return clock_cycle;
}

/// @}
