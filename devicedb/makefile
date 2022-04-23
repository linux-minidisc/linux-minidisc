INPUTS := convert.py minidisc.devids

OUTPUTS := ../qhimdtransfer/linux/etc/udev/60-minidisc.rules
OUTPUTS += ../libnetmd/netmd_dev_ids.h
OUTPUTS += ../netmd/libnetmd_dev_ids.py
OUTPUTS += netmd-js/netmd.devices.ts
OUTPUTS += ../qhimdtransfer/qhimddetection_dev_ids.cpp ../qhimdtransfer/qhimddetection_dev_ids.h

all: $(OUTPUTS)

$(OUTPUTS): $(INPUTS)
	python3 $<

.PHONY: all
