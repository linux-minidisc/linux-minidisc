INPUTS := convert.py minidisc.devids

OUTPUTS := udev/60-minidisc.rules
OUTPUTS += netmd-js/netmd.devices.ts

OUTPUTS += ../libnetmd/netmd_dev_ids.h
OUTPUTS += ../netmd/libnetmd_dev_ids.py
OUTPUTS += ../qhimdtransfer/qhimddetection_dev_ids.cpp ../qhimdtransfer/qhimddetection_dev_ids.h

all: $(OUTPUTS)

$(OUTPUTS): $(INPUTS)
	python3 $<

.PHONY: all
