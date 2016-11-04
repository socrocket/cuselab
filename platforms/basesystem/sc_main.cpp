// vim : set fileencoding=utf-8 expandtab noai ts=4 sw=4 :
/// @addtogroup platform
/// @{
/// @file sc_main.cpp
///
/// @date 2010-2015
/// @copyright All rights reserved.
///            Any reproduction, use, distribution or disclosure of this
///            program, without the express, prior written consent of the 
///            authors is strictly prohibited.
/// @author Bastian Farkas 
///
/*#ifdef HAVE_USI
#include "pysc/usi.h"
#endif*/

#include "core/common/systemc.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <mcheck.h>
#include "core/common/amba.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <signal.h>

#include "core/common/verbose.h"
#include "gaisler/memory/memory.h"
#include "gaisler/apbctrl/apbctrl.h"
#include "gaisler/ahbmem/ahbmem.h"
#include "gaisler/ahbctrl/ahbctrl.h"
#include <boost/filesystem.hpp>

#ifdef HAVE_AHBDISPLAY
#include "cuselab/models/ahbdisplay/ahbdisplay.h"
#endif
#ifdef HAVE_AHBCAMERA
#include "cuselab/models/ahbcamera/ahbcamera.h"
#endif
#include "cuselab/models/ahbgrayframer/ahbgrayframer.h"
#include "cuselab/models/ahbframetrigger/ahbframetrigger.h"

using namespace std;
using namespace sc_core;

void stopSimFunction(int sig) {
  v::warn << "main" << "Simulation interrupted by user" << std::endl;
  sc_core::sc_stop();
  wait(SC_ZERO_TIME);
}

int sc_main(int argc, char** argv) {
    clock_t cstart, cend;
    std::string prom_app;

    gs::ctr::GC_Core       core;
    gs::cnf::ConfigDatabase cnfdatabase("ConfigDatabase");
    gs::cnf::ConfigPlugin configPlugin(&cnfdatabase);
    
    SR_INCLUDE_MODULE(ArrayStorage);
    SR_INCLUDE_MODULE(MapStorage);
    
    // Build GreenControl Configuration Namespace
    // ==========================================
    gs::gs_param_array p_conf("conf");
    gs::gs_param_array p_system("system", p_conf);

    // Decide whether LT or AT
    gs::gs_param<bool> p_system_at("at", false, p_system);
    gs::gs_param<unsigned int> p_system_ncpu("ncpu", 1, p_system);
    gs::gs_param<unsigned int> p_system_clock("clk", 10.0, p_system);
    gs::gs_param<std::string> p_system_osemu("osemu", "", p_system);
    gs::gs_param<std::string> p_system_log("log", "", p_system);

    gs::gs_param_array p_report("report", p_conf);
    gs::gs_param<bool> p_report_timing("timing", true, p_report);
    gs::gs_param<bool> p_report_power("power", true, p_report);
   
    sc_signal<bool> cameraFrameSignal,gray0FrameSignal;
    sc_signal<char> keyCodeSignal;
    
    uint32_t videoWidth = 320;
    uint32_t frameWidth = 960;
    uint32_t frameHeight = 720;
     
    amba::amba_layer_ids ambaLayer;
    if(p_system_at) {
        ambaLayer = amba::amba_AT;
    } else {
        ambaLayer = amba::amba_LT;
    }

    // *** CREATE MODULES

    // AHBCtrl
    // =======
    // Always enabled.
    // Needed for basic platform
    gs::gs_param_array p_ahbctrl("ahbctrl", p_conf);
    gs::gs_param<unsigned int> p_ahbctrl_ioaddr("ioaddr", 0xFFF, p_ahbctrl);
    gs::gs_param<unsigned int> p_ahbctrl_iomask("iomask", 0xFFF, p_ahbctrl);
    gs::gs_param<unsigned int> p_ahbctrl_cfgaddr("cfgaddr", 0xFF0, p_ahbctrl);
    gs::gs_param<unsigned int> p_ahbctrl_cfgmask("cfgmask", 0xFF0, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_rrobin("rrobin", false, p_ahbctrl);
    gs::gs_param<unsigned int> p_ahbctrl_defmast("defmast", 0u, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_ioen("ioen", true, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_fixbrst("fixbrst", false, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_split("split", false, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_fpnpen("fpnpen", true, p_ahbctrl);
    gs::gs_param<bool> p_ahbctrl_mcheck("mcheck", true, p_ahbctrl);

    AHBCtrl ahbctrl("ahbctrl",
		    p_ahbctrl_ioaddr,                // The MSB address of the I/O area
		    p_ahbctrl_iomask,                // The I/O area address mask
		    p_ahbctrl_cfgaddr,               // The MSB address of the configuration area
		    p_ahbctrl_cfgmask,               // The address mask for the configuration area
		    p_ahbctrl_rrobin,                // 1 - round robin, 0 - fixed priority arbitration (only AT)
		    p_ahbctrl_split,                 // Enable support for AHB SPLIT response (only AT)
		    p_ahbctrl_defmast,               // Default AHB master
		    p_ahbctrl_ioen,                  // AHB I/O area enable
		    p_ahbctrl_fixbrst,               // Enable support for fixed-length bursts (disabled)
		    p_ahbctrl_fpnpen,                // Enable full decoding of PnP configuration records
		    p_ahbctrl_mcheck,                // Check if there are any intersections between core memory regions
        p_report_power,                  // Enable/disable power monitoring
		    ambaLayer
    );

    // Set clock
    ahbctrl.set_clk(p_system_clock, SC_NS);

    // AHBSlave - APBCtrl
    // ==================

    gs::gs_param_array p_apbctrl("apbctrl", p_conf);
    gs::gs_param<unsigned int> p_apbctrl_haddr("haddr", 0x800, p_apbctrl);
    gs::gs_param<unsigned int> p_apbctrl_hmask("hmask", 0xFFF, p_apbctrl);
    gs::gs_param<unsigned int> p_apbctrl_index("hindex", 2u, p_apbctrl);
    gs::gs_param<bool> p_apbctrl_check("mcheck", true, p_apbctrl);

    APBCtrl apbctrl("apbctrl",
        p_apbctrl_haddr,    // The 12 bit MSB address of the AHB area.
        p_apbctrl_hmask,    // The 12 bit AHB area address mask
        p_apbctrl_check,    // Check for intersections in the memory map
        p_apbctrl_index,    // AHB bus index
        p_report_power,     // Power Monitoring on/off
		    ambaLayer           // TLM abstraction layer
    );

    // Connect to AHB and clock
    ahbctrl.ahbOUT(apbctrl.ahb);
    apbctrl.set_clk(p_system_clock, SC_NS);

    // AHBSlave - AHBMem
    // =================
    gs::gs_param_array p_ahbmem("ahbmem", p_conf);
    gs::gs_param<bool> p_ahbmem_en("en", true, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_addr("addr", 0xA00, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_mask("mask", 0xE00, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_index("index", 1, p_ahbmem);
    gs::gs_param<bool> p_ahbmem_cacheable("cacheable", 1, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_waitstates("waitstates", 0u, p_ahbmem);
    gs::gs_param<std::string> p_ahbmem_elf("elf", "", p_ahbmem);

    if(p_ahbmem_en) {

      AHBMem *ahbmem = new AHBMem("ahbmem",
                                  p_ahbmem_addr,
                                  p_ahbmem_mask,
                                  ambaLayer,
                                  p_ahbmem_index,
                                  p_ahbmem_cacheable,
                                  p_ahbmem_waitstates,
                                  p_report_power

      );

      // Connect to ahbctrl and clock
      ahbctrl.ahbOUT(ahbmem->ahb);
      ahbmem->set_clk(p_system_clock, SC_NS);
    }


    // AHBMaster - AHBFrameTrigger (input_device)
    // ================================
    gs::gs_param_array p_ahbframetrigger("ahbframetrigger", p_conf);
    gs::gs_param<bool> p_ahbframetrigger_en("en", true, p_ahbframetrigger);
    gs::gs_param<unsigned int> p_ahbframetrigger_index("index", 1, p_ahbframetrigger);
    gs::gs_param<unsigned int> p_ahbframetrigger_interval("interval", 1, p_ahbframetrigger);
    if(p_ahbframetrigger_en) {
        AHBFrameTrigger *ahbframetrigger = new AHBFrameTrigger("ahbframetrigger",
          p_ahbframetrigger_index,
          sc_core::sc_time(p_ahbframetrigger_interval, SC_MS),
          p_report_power,
          ambaLayer
      );

      // Connect sensor to bus
      ahbframetrigger->ahb(ahbctrl.ahbIN);
      ahbframetrigger->set_clk(p_system_clock, SC_NS);
    }

#ifdef HAVE_AHBDISPLAY
    // AHBDisplay - AHBMaster
    // ==================
    gs::gs_param_array p_ahbdisplay("ahbdisplay", p_conf);
    gs::gs_param<bool> p_ahbdisplay_en("en", true, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_index("index", 2, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_pindex("pindex", 4, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_paddr("paddr", 0x500, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_pmask("pmask", 0xFFF, p_ahbdisplay);
    AHBDisplay *ahbdisplay;
    if(p_ahbdisplay_en) {
      ahbdisplay = new AHBDisplay("ahbdisplay",
        p_ahbdisplay_index,  // ahb index
        p_ahbdisplay_pindex,  // apb index
        p_ahbdisplay_paddr,  // apb address
        p_ahbdisplay_pmask,  // apb mask
        frameWidth, frameHeight
        //ambaLayer
      );

      // Connecting APB Slave
      ahbdisplay->ahb(ahbctrl.ahbIN);
      apbctrl.apb(ahbdisplay->apb);
      ahbdisplay->set_clk(p_system_clock,SC_NS);
      ahbdisplay->triggerIn(gray0FrameSignal);
      ahbdisplay->keyboardOut(keyCodeSignal);
    }
#endif
#ifdef HAVE_AHBCAMERA
    // AHBCamera - AHBMaster
    // ==================
    gs::gs_param_array p_ahbcamera("ahbcamera", p_conf);
    gs::gs_param<bool> p_ahbcamera_en("en", true, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_hindex("hindex", 3, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_pindex("pindex", 5, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_paddr("paddr", 0x501, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_pmask("pmask", 0xFFF, p_ahbcamera);
    gs::gs_param<std::string> p_ahbcamera_video("video", "bigbuckbunny_small_short.m2v", p_ahbcamera);
    if(p_ahbcamera_en) {
      AHBCamera *ahbcamera = new AHBCamera("ahbcamera",
        p_ahbcamera_hindex,  // ahb index
        p_ahbcamera_pindex,  // apb index
        p_ahbcamera_paddr,   // apb addr
        p_ahbcamera_pmask,   // apb make
        frameWidth, frameHeight,
        p_ahbcamera_video.get().c_str(),
        ambaLayer
      );

      // Connecting APB Slave
      ahbcamera->ahb(ahbctrl.ahbIN);
      apbctrl.apb(ahbcamera->apb);
      ahbcamera->set_clk(p_system_clock,SC_NS);
      // Connecting FrameTrigger Port
      ahbcamera->triggerOut(cameraFrameSignal);
    }
#endif

    // AHBGrayframer - AHBMaster
    // ==================
    gs::gs_param_array p_ahbgrayframer0("ahbgrayframer0", p_conf);
    gs::gs_param<bool> p_ahbgrayframer0_en("en", true, p_ahbgrayframer0);
    gs::gs_param<unsigned int> p_ahbgrayframer0_hindex("hindex", 4, p_ahbgrayframer0);
    gs::gs_param<unsigned int> p_ahbgrayframer0_pindex("pindex", 6, p_ahbgrayframer0);
    gs::gs_param<unsigned int> p_ahbgrayframer0_paddr("paddr", 0x502, p_ahbgrayframer0);
    gs::gs_param<unsigned int> p_ahbgrayframer0_pmask("pmask", 0xFFF, p_ahbgrayframer0);
    if(p_ahbgrayframer0_en) {
      AHBGrayframer *ahbgrayframer0 = new AHBGrayframer("ahbgrayframer0",
        p_ahbgrayframer0_hindex,  // ahb index
        p_ahbgrayframer0_pindex,  // apb index
        p_ahbgrayframer0_paddr,   // apb addr
        p_ahbgrayframer0_pmask,   // apb make
        'Y',
        0,0,
        320,0,
        videoWidth,
        frameWidth,frameHeight,
        ambaLayer
      );

      // Connecting APB Slave
      ahbgrayframer0->ahb(ahbctrl.ahbIN);
      apbctrl.apb(ahbgrayframer0->apb);
      ahbgrayframer0->set_clk(p_system_clock,SC_NS);
      ahbgrayframer0->triggerIn(cameraFrameSignal);
      ahbgrayframer0->triggerOut(gray0FrameSignal);
    }

    // disable Info messages
    sc_report_handler::set_actions(SC_INFO, SC_DO_NOTHING);

    (void) signal(SIGINT, stopSimFunction);
    (void) signal(SIGTERM, stopSimFunction);
    cstart = cend = clock();
    cstart = clock();
    //mtrace();

    sc_core::sc_start();
    //muntrace();
    cend = clock();

    v::info << "Summary" << "Start: " << dec << cstart << v::endl;
    v::info << "Summary" << "End:   " << dec << cend << v::endl;
    v::info << "Summary" << "Delta: " << dec << setprecision(4) << ((double)(cend - cstart) / (double)CLOCKS_PER_SEC * 1000) << "ms" << v::endl;

    std::cout << "End of sc_main" << std::endl << std::flush;
    return 0;
}
/// @}
