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
#ifdef HAVE_USI
#include "pysc/usi.h"
#endif

#include "core/common/sr_param.h"
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

#include "core/common/verbose.h"
#include "gaisler/memory/memory.h"
#include "gaisler/leon3/leon3.h"
#include "gaisler/memory/memory.h"
#include "gaisler/apbctrl/apbctrl.h"
#include "gaisler/mctrl/mctrl.h"
#include "gaisler/leon3/mmucache/defines.h"
#include "gaisler/gptimer/gptimer.h"
#include "gaisler/apbuart/apbuart.h"
#include "gaisler/apbuart/tcpio.h"
#include "gaisler/apbuart/reportio.h"
#include "gaisler/irqmp/irqmp.h"
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
#include "cuselab/models/apbkeyboard/apbkeyboard.h"

using namespace std;
using namespace sc_core;

void stopSimFunction(int sig) {
  v::warn << "main" << "Simulation interrupted by user" << std::endl;
  sc_core::sc_stop();
  wait(SC_ZERO_TIME);
}

class irqmp_rst_stimuli : sc_core::sc_module {
  public:
    sr_signal::signal_out<bool, Irqmp> irqmp_rst;
    irqmp_rst_stimuli(sc_core::sc_module_name mn) : 
        sc_core::sc_module(mn), 
        irqmp_rst("rst") {

    }
    void start_of_simulation() {
      irqmp_rst.write(0);
      irqmp_rst.write(1);
    }
};

int sc_main(int argc, char** argv) {
    clock_t cstart, cend;
    std::string prom_app;

    gs::ctr::GC_Core       core;
    gs::cnf::ConfigDatabase cnfdatabase("ConfigDatabase");
    gs::cnf::ConfigPlugin configPlugin(&cnfdatabase);
    
    SR_INCLUDE_MODULE(ArrayStorage);
    SR_INCLUDE_MODULE(MapStorage);
    SR_INCLUDE_MODULE(ReportIO);
    SR_INCLUDE_MODULE(TcpIO);


#ifdef HAVE_USI
    // Initialize Python
    USI_HAS_MODULE(systemc);
    USI_HAS_MODULE(sr_registry);
    USI_HAS_MODULE(delegate);
    USI_HAS_MODULE(intrinsics);
    USI_HAS_MODULE(greensocket);
    USI_HAS_MODULE(scireg);
    USI_HAS_MODULE(amba);
    USI_HAS_MODULE(sr_report);
    USI_HAS_MODULE(cci);
    USI_HAS_MODULE(mtrace);
    usi_init(argc, argv);
    //sr_report_handler::handler = sr_report_handler::default_handler; <<-- Uncoment for C++ handler


    // Core APIs will be loaded by usi_init:
    // usi, usi.systemc, usi.api.delegate, usi.api.report
    usi_load("usi.api.greensocket");
    usi_load("sr_register.scireg");
    usi_load("usi.api.amba");

    //usi_load("usi.log.console_reporter");
    usi_load("usi.tools.args");
    usi_load("usi.cci");
    //usi_load("tools.python.power");
    usi_load("usi.shell");
    usi_load("usi.tools.execute");
    usi_load("usi.tools.elf");

    usi_start_of_initialization();
#endif  // HAVE_USI
    
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
   
    sc_signal<bool> cameraFrameSignal,grayFrameSignal;
    sc_signal<char> keyCodeSignal;
     
    uint32_t videoWidth = 320;
    uint32_t frameWidth = 640;
    uint32_t frameHeight = 480;
     
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

    // APBSlave - IRQMP
    // ================
    // Needed for basic platform.
    // Always enabled

    gs::gs_param_array p_irqmp("irqmp", p_conf);
    gs::gs_param<unsigned int> p_irqmp_addr("addr", /*0x1F0*/0x2, p_irqmp); // SoCRocket default: 0x1F0, try to mimic TSIM therefore 0x2 -- psiegl
    gs::gs_param<unsigned int> p_irqmp_mask("mask", 0xFFF, p_irqmp);
    gs::gs_param<unsigned int> p_irqmp_index("index", 2, p_irqmp);
    gs::gs_param<unsigned int> p_irqmp_eirq("eirq", 0u, p_irqmp);

    Irqmp irqmp("irqmp",
                p_irqmp_addr,  // paddr
                p_irqmp_mask,  // pmask
                p_system_ncpu,  // ncpu
                p_irqmp_eirq,  // eirq
                p_irqmp_index, // pindex
                p_report_power // power monitoring
    );

    // Connect to APB and clock
    apbctrl.apb(irqmp.apb);
    irqmp.set_clk(p_system_clock,SC_NS);

    // AHBSlave - MCtrl, ArrayMemory
    // =============================
    gs::gs_param_array p_mctrl("mctrl", p_conf);
    gs::gs_param_array p_mctrl_apb("apb", p_mctrl);
    gs::gs_param_array p_mctrl_prom("prom", p_mctrl);
    gs::gs_param_array p_mctrl_io("io", p_mctrl);
    gs::gs_param_array p_mctrl_ram("ram", p_mctrl);
    gs::gs_param_array p_mctrl_ram_sram("sram", p_mctrl_ram);
    gs::gs_param_array p_mctrl_ram_sdram("sdram", p_mctrl_ram);
    gs::gs_param<unsigned int> p_mctrl_apb_addr("addr", 0x000u, p_mctrl_apb);
    gs::gs_param<unsigned int> p_mctrl_apb_mask("mask", 0xFFF, p_mctrl_apb);
    gs::gs_param<unsigned int> p_mctrl_apb_index("index", 0u, p_mctrl_apb);
    gs::gs_param<unsigned int> p_mctrl_prom_addr("addr", 0x000u, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_prom_mask("mask", 0xE00, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_prom_asel("asel", 28, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_prom_banks("banks", 2, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_prom_bsize("bsize", 256, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_prom_width("width", 32, p_mctrl_prom);
    gs::gs_param<unsigned int> p_mctrl_io_addr("addr", 0x200, p_mctrl_io);
    gs::gs_param<unsigned int> p_mctrl_io_mask("mask", 0xE00, p_mctrl_io);
    gs::gs_param<unsigned int> p_mctrl_io_banks("banks", 1, p_mctrl_io);
    gs::gs_param<unsigned int> p_mctrl_io_bsize("bsize", 512, p_mctrl_io);
    gs::gs_param<unsigned int> p_mctrl_io_width("width", 32, p_mctrl_io);
    gs::gs_param<unsigned int> p_mctrl_ram_addr("addr", 0x400, p_mctrl_ram);
    gs::gs_param<unsigned int> p_mctrl_ram_mask("mask", 0xC00, p_mctrl_ram);
    gs::gs_param<bool> p_mctrl_ram_wprot("wprot", false, p_mctrl_ram);
    gs::gs_param<unsigned int> p_mctrl_ram_asel("asel", 29, p_mctrl_ram);
    gs::gs_param<unsigned int> p_mctrl_ram_sram_banks("banks", 4, p_mctrl_ram_sram);
    gs::gs_param<unsigned int> p_mctrl_ram_sram_bsize("bsize", 128, p_mctrl_ram_sram);
    gs::gs_param<unsigned int> p_mctrl_ram_sram_width("width", 32, p_mctrl_ram_sram);
    gs::gs_param<unsigned int> p_mctrl_ram_sdram_banks("banks", 2, p_mctrl_ram_sdram);
    gs::gs_param<unsigned int> p_mctrl_ram_sdram_bsize("bsize", 256, p_mctrl_ram_sdram);
    gs::gs_param<unsigned int> p_mctrl_ram_sdram_width("width", 32, p_mctrl_ram_sdram);
    gs::gs_param<unsigned int> p_mctrl_ram_sdram_cols("cols", 16, p_mctrl_ram_sdram);
    gs::gs_param<unsigned int> p_mctrl_index("index", 0u, p_mctrl);
    gs::gs_param<bool> p_mctrl_ram8("ram8", true, p_mctrl);
    gs::gs_param<bool> p_mctrl_ram16("ram16", true, p_mctrl);
    gs::gs_param<bool> p_mctrl_sden("sden", true, p_mctrl);
    gs::gs_param<bool> p_mctrl_sepbus("sepbus", false, p_mctrl);
    gs::gs_param<unsigned int> p_mctrl_sdbits("sdbits", 32, p_mctrl);
    gs::gs_param<unsigned int> p_mctrl_mobile("mobile", 0u, p_mctrl);
    Mctrl mctrl( "mctrl",
        p_mctrl_prom_asel,
        p_mctrl_ram_asel,
        p_mctrl_prom_addr,
        p_mctrl_prom_mask,
        p_mctrl_io_addr,
        p_mctrl_io_mask,
        p_mctrl_ram_addr,
        p_mctrl_ram_mask,
        p_mctrl_apb_addr,
        p_mctrl_apb_mask,
        p_mctrl_ram_wprot,
        p_mctrl_ram_sram_banks,
        p_mctrl_ram8,
        p_mctrl_ram16,
        p_mctrl_sepbus,
        p_mctrl_sdbits,
        p_mctrl_mobile,
        p_mctrl_sden,
        p_mctrl_index,
        p_mctrl_apb_index,
        p_report_power,
        ambaLayer
    );

    // Connecting AHB Slave
    ahbctrl.ahbOUT(mctrl.ahb);
    // Connecting APB Slave
    apbctrl.apb(mctrl.apb);
    // Set clock
    mctrl.set_clk(p_system_clock, SC_NS);

    // CREATE MEMORIES
    // ===============

    // ROM instantiation
    Memory rom( "rom",
                     MEMDevice::ROM,
                     p_mctrl_prom_banks,
                     p_mctrl_prom_bsize * 1024 * 1024,
                     p_mctrl_prom_width,
                     0,
                     "ArrayStorage",
                     p_report_power
    );

    // Connect to memory controller and clock
    mctrl.mem(rom.bus);
    rom.set_clk(p_system_clock, SC_NS);

    // IO memory instantiation
    Memory io( "io",
               MEMDevice::IO,
               p_mctrl_prom_banks,
               p_mctrl_prom_bsize * 1024 * 1024,
               p_mctrl_prom_width,
               0,
               "MapStorage",
               p_report_power
    );

    // Connect to memory controller and clock
    mctrl.mem(io.bus);
    io.set_clk(p_system_clock, SC_NS);

    // ELF loader from leon (Trap-Gen)
    gs::gs_param<std::string> p_mctrl_io_elf("elf", "", p_mctrl_io);

    // SRAM instantiation
    Memory sram( "sram",
                 MEMDevice::SRAM,
                 p_mctrl_ram_sram_banks,
                 p_mctrl_ram_sram_bsize * 1024 * 1024,
                 p_mctrl_ram_sram_width,
                 0,
                 "MapStorage",
                 p_report_power
    );

    // Connect to memory controller and clock
    mctrl.mem(sram.bus);
    sram.set_clk(p_system_clock, SC_NS);

    // ELF loader from leon (Trap-Gen)
    gs::gs_param<std::string> p_mctrl_ram_sram_elf("elf", "", p_mctrl_ram_sram);

    // SDRAM instantiation
    Memory sdram( "sdram",
                       MEMDevice::SDRAM,
                       p_mctrl_ram_sdram_banks,
                       p_mctrl_ram_sdram_bsize * 1024 * 1024,
                       p_mctrl_ram_sdram_width,
                       p_mctrl_ram_sdram_cols,
                       "ArrayStorage",
                       p_report_power
    );

    // Connect to memory controller and clock
    mctrl.mem(sdram.bus);
    sdram.set_clk(p_system_clock, SC_NS);

    // ELF loader from leon (Trap-Gen)
    gs::gs_param<std::string> p_mctrl_ram_sdram_elf("elf", "", p_mctrl_ram_sdram);


    //leon3.ENTRY_POINT   = 0;
    //leon3.PROGRAM_LIMIT = 0;
    //leon3.PROGRAM_START = 0;

    // AHBSlave - AHBMem
    // =================
    gs::gs_param_array p_ahbmem("ahbmem", p_conf);
    gs::gs_param<bool> p_ahbmem_en("en", true, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_addr("addr", 0xA00, p_ahbmem);
    gs::gs_param<unsigned int> p_ahbmem_mask("mask", 0xFFF, p_ahbmem);
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

    // CREATE LEON3 Processor
    // ===================================================
    // Always enabled.
    // Needed for basic platform.
    gs::gs_param_array p_mmu_cache("mmu_cache", p_conf);
    gs::gs_param_array p_mmu_cache_ic("ic", p_mmu_cache);
    gs::gs_param<bool> p_mmu_cache_ic_en("en", true, p_mmu_cache_ic);
    gs::gs_param<int> p_mmu_cache_ic_repl("repl", 0, p_mmu_cache_ic);
    gs::gs_param<int> p_mmu_cache_ic_sets("sets", 1, p_mmu_cache_ic);
    gs::gs_param<int> p_mmu_cache_ic_linesize("linesize", 8, p_mmu_cache_ic);
    gs::gs_param<int> p_mmu_cache_ic_setsize("setsize", 4, p_mmu_cache_ic);
    gs::gs_param<bool> p_mmu_cache_ic_setlock("setlock", 1, p_mmu_cache_ic);
    gs::gs_param_array p_mmu_cache_dc("dc", p_mmu_cache);
    gs::gs_param<bool> p_mmu_cache_dc_en("en", true, p_mmu_cache_dc);
    gs::gs_param<int> p_mmu_cache_dc_repl("repl", 0, p_mmu_cache_dc);
    gs::gs_param<int> p_mmu_cache_dc_sets("sets", 1, p_mmu_cache_dc);
    gs::gs_param<int> p_mmu_cache_dc_linesize("linesize", 4, p_mmu_cache_dc);
    gs::gs_param<int> p_mmu_cache_dc_setsize("setsize", 4, p_mmu_cache_dc);
    gs::gs_param<bool> p_mmu_cache_dc_setlock("setlock", 1, p_mmu_cache_dc);
    gs::gs_param<bool> p_mmu_cache_dc_snoop("snoop", 1, p_mmu_cache_dc);
    gs::gs_param_array p_mmu_cache_ilram("ilram", p_mmu_cache);
    gs::gs_param<bool> p_mmu_cache_ilram_en("en", false, p_mmu_cache_ilram);
    gs::gs_param<unsigned int> p_mmu_cache_ilram_size("size", 0u, p_mmu_cache_ilram);
    gs::gs_param<unsigned int> p_mmu_cache_ilram_start("start", 0u, p_mmu_cache_ilram);
    gs::gs_param_array p_mmu_cache_dlram("dlram", p_mmu_cache);
    gs::gs_param<bool> p_mmu_cache_dlram_en("en", false, p_mmu_cache_dlram);
    gs::gs_param<unsigned int> p_mmu_cache_dlram_size("size", 0u, p_mmu_cache_dlram);
    gs::gs_param<unsigned int> p_mmu_cache_dlram_start("start", 0u, p_mmu_cache_dlram);
    gs::gs_param<unsigned int> p_mmu_cache_cached("cached", 0u, p_mmu_cache);
    gs::gs_param<unsigned int> p_mmu_cache_index("index", 0u, p_mmu_cache);
    gs::gs_param_array p_mmu_cache_mmu("mmu", p_mmu_cache);
    gs::gs_param<bool> p_mmu_cache_mmu_en("en", true, p_mmu_cache_mmu);
    gs::gs_param<unsigned int> p_mmu_cache_mmu_itlb_num("itlb_num", 8, p_mmu_cache_mmu);
    gs::gs_param<unsigned int> p_mmu_cache_mmu_dtlb_num("dtlb_num", 8, p_mmu_cache_mmu);
    gs::gs_param<unsigned int> p_mmu_cache_mmu_tlb_type("tlb_type", 1u, p_mmu_cache_mmu);
    gs::gs_param<unsigned int> p_mmu_cache_mmu_tlb_rep("tlb_rep", 1, p_mmu_cache_mmu);
    gs::gs_param<unsigned int> p_mmu_cache_mmu_mmupgsz("mmupgsz", 0u, p_mmu_cache_mmu);

    gs::gs_param<std::string> p_proc_history("history", "", p_system);

    gs::gs_param_array p_gdb("gdb", p_conf);
    gs::gs_param<bool> p_gdb_en("en", false, p_gdb);
    gs::gs_param<int> p_gdb_port("port", 1500, p_gdb);
    gs::gs_param<int> p_gdb_proc("proc", 0, p_gdb);
    Leon3 *first_leon = NULL;
    for(uint32_t i=0; i< p_system_ncpu; i++) {
      // AHBMaster - MMU_CACHE
      // =====================
      // Always enabled.
      // Needed for basic platform.
      Leon3 *leon3 = new Leon3(
              sc_core::sc_gen_unique_name("leon3", false), // name of sysc module
              p_mmu_cache_ic_en,         //  int icen = 1 (icache enabled)
              p_mmu_cache_ic_repl,       //  int irepl = 0 (icache LRU replacement)
              p_mmu_cache_ic_sets,       //  int isets = 4 (4 instruction cache sets)
              p_mmu_cache_ic_linesize,   //  int ilinesize = 4 (4 words per icache line)
              p_mmu_cache_ic_setsize,    //  int isetsize = 16 (16kB per icache set)
              p_mmu_cache_ic_setlock,    //  int isetlock = 1 (icache locking enabled)
              p_mmu_cache_dc_en,         //  int dcen = 1 (dcache enabled)
              p_mmu_cache_dc_repl,       //  int drepl = 2 (dcache random replacement)
              p_mmu_cache_dc_sets,       //  int dsets = 2 (2 data cache sets)
              p_mmu_cache_dc_linesize,   //  int dlinesize = 4 (4 word per dcache line)
              p_mmu_cache_dc_setsize,    //  int dsetsize = 1 (1kB per dcache set)
              p_mmu_cache_dc_setlock,    //  int dsetlock = 1 (dcache locking enabled)
              p_mmu_cache_dc_snoop,      //  int dsnoop = 1 (dcache snooping enabled)
              p_mmu_cache_ilram_en,      //  int ilram = 0 (instr. localram disable)
              p_mmu_cache_ilram_size,    //  int ilramsize = 0 (1kB ilram size)
              p_mmu_cache_ilram_start,   //  int ilramstart = 8e (0x8e000000 default ilram start address)
              p_mmu_cache_dlram_en,      //  int dlram = 0 (data localram disable)
              p_mmu_cache_dlram_size,    //  int dlramsize = 0 (1kB dlram size)
              p_mmu_cache_dlram_start,   //  int dlramstart = 8f (0x8f000000 default dlram start address)
              p_mmu_cache_cached,        //  int cached = 0xffff (fixed cacheability mask)
              p_mmu_cache_mmu_en,        //  int mmu_en = 0 (mmu not present)
              p_mmu_cache_mmu_itlb_num,  //  int itlb_num = 8 (8 itlbs - not present)
              p_mmu_cache_mmu_dtlb_num,  //  int dtlb_num = 8 (8 dtlbs - not present)
              p_mmu_cache_mmu_tlb_type,  //  int tlb_type = 0 (split tlb mode - not present)
              p_mmu_cache_mmu_tlb_rep,   //  int tlb_rep = 1 (random replacement)
              p_mmu_cache_mmu_mmupgsz,   //  int mmupgsz = 0 (4kB mmu page size)>
              p_mmu_cache_index + i,     // Id of the AHB master
              p_report_power,            // Power Monitor,
              ambaLayer                  // TLM abstraction layer
      );
      if(!first_leon) {
        first_leon = leon3;
      }

      // Connecting AHB Master
      leon3->ahb(ahbctrl.ahbIN);
      // Set clock
      leon3->set_clk(p_system_clock, SC_NS);
      connect(leon3->snoop, ahbctrl.snoop);

      // History logging
      std::string history = p_proc_history;
      if(!history.empty()) {
        leon3->g_history = history;
      }

      connect(irqmp.irq_req, leon3->cpu.IRQ_port.irq_signal, i);
      connect(leon3->cpu.irqAck.initSignal, irqmp.irq_ack, i);
      connect(leon3->cpu.irqAck.run, irqmp.cpu_rst, i);
      connect(leon3->cpu.irqAck.status, irqmp.cpu_stat, i);

      // GDBStubs
      if(p_gdb_en) {
        leon3->g_gdb = p_gdb_port;
      }
      // OS Emulator
      // ===========
      // is activating the leon traps to map basic io functions to the host system
      // set_brk, open, read, ...
      if(!((std::string)p_system_osemu).empty()) {
        leon3->g_osemu = p_system_osemu;
      }
    }

    // APBSlave - GPTimer
    // ==================
    gs::gs_param_array p_gptimer("gptimer", p_conf);
    gs::gs_param<bool> p_gptimer_en("en", true, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_ntimers("ntimers", 2, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_pindex("pindex", 3, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_paddr("paddr", 0x3, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_pmask("pmask", 0xfff, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_pirq("pirq", 8, p_gptimer);
    gs::gs_param<bool> p_gptimer_sepirq("sepirq", true, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_sbits("sbits", 16, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_nbits("nbits", 32, p_gptimer);
    gs::gs_param<unsigned int> p_gptimer_wdog("wdog", 1u, p_gptimer);

    if(p_gptimer_en) {
      GPTimer *gptimer = new GPTimer("gptimer",
        p_gptimer_ntimers,  // ntimers
        p_gptimer_pindex,   // index
        p_gptimer_paddr,    // paddr
        p_gptimer_pmask,    // pmask
        p_gptimer_pirq,     // pirq
        p_gptimer_sepirq,   // sepirq
        p_gptimer_sbits,    // sbits
        p_gptimer_nbits,    // nbits
        p_gptimer_wdog,     // wdog
        p_report_power      // powmon
      );

      // Connect to apb and clock
      apbctrl.apb(gptimer->apb);
      gptimer->set_clk(p_system_clock,SC_NS);

      // Connecting Interrupts
      for(int i=0; i < 8; i++) {
        sr_signal::connect(irqmp.irq_in, gptimer->irq, p_gptimer_pirq + i);
      }

    }

#ifdef HAVE_AHBDISPLAY
    // AHBDisplay - AHBMaster
    // ==================
    gs::gs_param_array p_ahbdisplay("ahbdisplay", p_conf);
    gs::gs_param<bool> p_ahbdisplay_en("en", true, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_index("index", 3, p_ahbdisplay);
    gs::gs_param<unsigned int> p_ahbdisplay_pindex("pindex", 5, p_ahbdisplay);
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
      ahbdisplay->triggerIn(grayFrameSignal);
      ahbdisplay->keyboardOut(keyCodeSignal);
    }
#endif
#ifdef HAVE_AHBCAMERA
    // AHBCamera - AHBMaster
    // ==================
    gs::gs_param_array p_ahbcamera("ahbcamera", p_conf);
    gs::gs_param<bool> p_ahbcamera_en("en", true, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_hindex("hindex", 4, p_ahbcamera);
    gs::gs_param<unsigned int> p_ahbcamera_pindex("pindex", 6, p_ahbcamera);
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
        ((std::string)p_ahbcamera_video).c_str(),
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
    gs::gs_param<unsigned int> p_ahbgrayframer0_hindex("hindex", 5, p_ahbgrayframer0);
    gs::gs_param<unsigned int> p_ahbgrayframer0_pindex("pindex", 7, p_ahbgrayframer0);
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
      ahbgrayframer0->triggerOut(grayFrameSignal);
    }

    // APBKeyboard - APBSlave
    // ==================
    gs::gs_param_array p_apbkeyboard("apbkeyboard", p_conf);
    gs::gs_param<bool> p_apbkeyboard_en("en", true, p_apbkeyboard);
    gs::gs_param<unsigned int> p_apbkeyboard_pindex("pindex", 8, p_apbkeyboard);
    gs::gs_param<unsigned int> p_apbkeyboard_paddr("paddr", 0x503, p_apbkeyboard);
    gs::gs_param<unsigned int> p_apbkeyboard_pmask("pmask", 0xFFF, p_apbkeyboard);
    gs::gs_param<unsigned int> p_apbkeyboard_pirq("pirq", 0u, p_apbkeyboard);
    if(p_apbkeyboard_en) {
      APBKeyboard *apbkeyboard = new APBKeyboard("apbkeyboard",
        p_apbkeyboard_pindex,  // apb index
        p_apbkeyboard_paddr,   // apb addr
        p_apbkeyboard_pmask,   // apb mask
        p_apbkeyboard_pirq   // apb irq
      );

      // Connecting APB Slave
      apbctrl.apb(apbkeyboard->apb);
      apbkeyboard->keyboardIn(keyCodeSignal);
    }

    irqmp_rst_stimuli stimuli("platform_stimuli");
    connect(stimuli.irqmp_rst, irqmp.rst);
    // disable Info messages
    sc_report_handler::set_actions(SC_INFO, SC_DO_NOTHING);

#ifndef HAVE_USI
    (void) signal(SIGINT, stopSimFunction);
    (void) signal(SIGTERM, stopSimFunction);
#endif
    cstart = cend = clock();
    cstart = clock();
    //mtrace();
#ifdef HAVE_USI
    usi_start();
#else
    sc_core::sc_start();
#endif
    //muntrace();
    cend = clock();

    v::info << "Summary" << "Start: " << dec << cstart << v::endl;
    v::info << "Summary" << "End:   " << dec << cend << v::endl;
    v::info << "Summary" << "Delta: " << dec << setprecision(4) << ((double)(cend - cstart) / (double)CLOCKS_PER_SEC * 1000) << "ms" << v::endl;

    std::cout << "End of sc_main" << std::endl << std::flush;
    return 0;
}
/// @}
