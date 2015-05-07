# RTAI IRQ Latency Experiments

## Companion article

Please read the
[companion article](http://thotypous.github.io/2015-05/rtai-measuring-irq-latency)
to this repository in order to understand its contents.

## Directory structure

Directories below contain a complete `Makefile` for compiling them.
You may need to pass environment variables if you use any non-standard paths for RTAI or Quartus II.

 * `LPT_SW` → RTAI module for testing parallel port interrupt latency
 * `PCIe_SW` → RTAI module for testing PCI express interrupt latency
 * `PCIe_HW` → Quartus II project and Bluespec source code for the PCIe hardware
 * `plot` → histogram plotting tool
