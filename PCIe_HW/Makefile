QUARTUS_ROOTDIR ?= /opt/altera/14.1/quartus

all: output_files/de4_pcie.sof

output_files/de4_pcie.sof: de4_pcie/synthesis/de4_pcie.qip
	$(QUARTUS_ROOTDIR)/bin/quartus_sh --flow compile de4_pcie

de4_pcie/synthesis/de4_pcie.qip: de4_pcie.qsys mkEchoModule.v
	$(QUARTUS_ROOTDIR)/sopc_builder/bin/qsys-generate --synthesis=VERILOG $<

mkEchoModule.v: $(wildcard BSV/*.bsv)
	echo $(QUARTUS_ROOTDIR)
	bsc -u -verilog -vdir . -bdir BSV  BSV/EchoModule.bsv

clean:
	rm -f BSV/*.bo mkEchoModule.v
	rm -rf de4_pcie de4_pcie.sopcinfo
	rm -rf db incremental_db output_files
	rm -f PLLJ_PLLSPE_INFO.txt
