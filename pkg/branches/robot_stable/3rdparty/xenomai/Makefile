all: build

XENOMAI_REVISION = 4220
SVN_DIR = xenomai-svn
SVN_URL = http://svn.gna.org/svn/xenomai/tags/v2.4.5 
SVN_REVISION = -r $(XENOMAI_REVISION)
SVN_PATCH = ipipe.patch
include $(shell rospack find mk)/svn_checkout.mk

build: SVN_UP_REVERT_PATCH xenomai

xenomai: changelist.txt linux
	cd $(SVN_DIR) && mkdir -p build && cd build && ../configure --prefix=$(PWD)/xenomai --enable-x86-sep --enable-x86-tsc --enable-smp --disable-dox-doc
	cd $(SVN_DIR) && \
	./scripts/prepare-kernel.sh --linux=../linux --arch=i386 \
		--adeos=ksrc/arch/x86/patches/adeos-ipipe-2.6.24-x86-2.0-07.patch 
	cd linux && \
	cp ../config-$(LINUX_VERSION)-AOpen .config && \
	make oldconfig && make prepare scripts
	cd $(SVN_DIR)/build && make $(PARALLEL_JOBS) SUDO=false install
	touch xenomai

clean:
	-cd $(SVN_DIR)/build && make clean
	rm -rf xenomai 

wipe: 
	rm -rf xenomai-svn xenomai


LINUX_VERSION = 2.6.24.3
empty :=
space := $(empty) $(empty)
MAJOR_VERSION = v$(subst $(space),.,$(wordlist 1, 2, $(subst .,$(space),$(LINUX_VERSION))))
LINUX_ARCHIVE = linux-$(LINUX_VERSION).tar.bz2

download_kernel: $(LINUX_ARCHIVE)

kernel: xenomai
	make  $(PARALLEL_JOBS) -C linux

install_kernel: xenomai
	sudo make  $(PARALLEL_JOBS) -C $(SVN_DIR)/build devices
	sudo make  $(PARALLEL_JOBS) -C linux install modules_install
	cd /boot && sudo update-initramfs -c -k 2.6.24.3 && sudo update-grub

$(LINUX_ARCHIVE): 
	wget http://pr.willowgarage.com/downloads/$(LINUX_ARCHIVE)

linux: $(LINUX_ARCHIVE)
	tar xjf $(LINUX_ARCHIVE)
	mv $(subst .tar.bz2,,$(LINUX_ARCHIVE)) $@
	touch linux

clean_kernel:
	cd linux && make clean

wipe_kernel: wipe
	rm -rf $(LINUX_ARCHIVE) linux

