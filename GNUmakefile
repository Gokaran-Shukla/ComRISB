include arch.mk

default: all

all: env com_wannier90 com_comgw com_comwann cy_gutz py_grisb

env:
	mkdir -p bin
	if ! grep -Fq "COMRISB_BIN=" ~/.bashrc; then \
		echo -e '\nexport COMRISB_BIN=${PWD}/bin' >> ~/.bashrc; \
		COMRISB_BIN=${PWD}/bin; \
	fi
com_wannier90:  	
	cd wannier90_3.1.0 && $(MAKE) lib && cd ..
com_comgw:
	cd gw && $(MAKE) && cd ..
com_comwann:
	cd ComWann && $(MAKE) && cd ..
cy_gutz:
	cd cygutz && $(MAKE) install && cd ..
py_grisb:
	cd pygrisb && $(MAKE) install && cd ..


clean: clean_wannier90 clean_comgw clean_comwann 

clean_comgw:
	cd gw && $(MAKE) clean && cd ..
clean_comwann:
	cd ComWann && $(MAKE) clean && cd ..
clean_wannier90:  	
	cd wannier90_3.1.0 && $(MAKE) clean && cd ..
clean_bin:
	rm -rf ./bin/*
