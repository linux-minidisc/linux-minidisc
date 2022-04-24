INPUTS := convert.py minidisc.devids

OUTPUTS := udev/60-minidisc.rules
OUTPUTS += netmd-js/netmd.devices.ts

OUTPUTS += ../netmd/libnetmd_dev_ids.py
OUTPUTS += ../libusbmd/libusbmd_data.h

all: $(OUTPUTS)

$(OUTPUTS): $(INPUTS)
	python3 $<

.PHONY: all
