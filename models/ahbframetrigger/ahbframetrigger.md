AHBFrameTrigger - AHB Demonstration FrameTrigger Device {#ahbframetrigger_p}
=================================================
[TOC]

@section ahbframetrigger_p1 Overview

The AHBFrameTrigger model generates input data for the AHB bus to controll AHBCamera, AHBDisplay and further models connected to the bus. 
The input data consists of fixed size random data frames, which are written to a certain address in a certain interval. 
The class inherits from the `AHBMaster` and `CLKDevice` classes. 
It has no VHDL reference in the Gaisler Library.

@section ahbframetrigger_p2 Interface

This component provides the typical AHB master generics refactored as constructor parameters of the class `AHBOut`. 
An overview about the available parameters is given in table 41.

@table Table 41 - AHBIN Constructor Parameters
| Parameter | Description                                     |
|-----------|-------------------------------------------------|
| name      | SystemC name of the module                      |
| hindex    | The master index for registering with the AHB   |
| hirq      | The number of the IRQ raised for available data |
| interval  | The interval between data frames                |
| pow_mon   | Enable power monitoring                         |
| ambaLayer | TLM abstraction layer                           |
@endtable

@section ahbframetrigger_p3 Example Instantiation

This example shows how to instantiate the module AHBIN. 
In line 590 the constructor is called to create the new object. 
In line 601 the module is connected to the bus and in the next line the clock is set. 
In line 605 the interrupt output is connected via Signalkit. 

~~~{.cpp}
AHBFrameTrigger *ahbframetrigger = new AHBIn("ahbframetrigger",
    p_ahbframetrigger_index,
    sc_core::sc_time(p_ahbframetrigger_interval, SC_MS),
    p_report_power,
    ambaLayer
);

// Connect module to bus
ahbframetrigger->ahb(ahbctrl.ahbIN);
ahbframetrigger->set_clk(p_system_clock, SC_NS);
~~~
