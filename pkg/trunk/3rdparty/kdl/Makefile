.PHONY: kdl

all: build

TARBALL = build/kdl_29562.tar.gz
TARBALL_URL = http://pr.willowgarage.com/downloads/kdl_29562.tar.gz
SOURCE_DIR = build/kdl_export
#MD5SUM_FILE = opencv-1.0.0.tar.gz.md5sum
UNPACK_CMD = tar xzf
include $(shell rospack find mk)/download_unpack_build.mk




PATCH1 = kdl_patch29562.diff
PATCH2 = kdl_patch_quaternion.diff
include $(shell rospack find mk)/svn_checkout.mk

CMAKE = `rospack find cmake`/cmake/bin/cmake 
BOOST_ROOT =$(shell rospack find boost)/boost
CMAKE_ARGS = -DCMAKE_INSTALL_PREFIX=$(PWD)/kdl/ \
		-DPYTHON_BINDINGS=OFF \
		-DKDL_PYTHON_INSTALL_PATH=$(PWD)/kdl/lib/ \
		-DBOOST_INCLUDEDIR:STRING=$(BOOST_ROOT)/include \
  	-DBOOST_LIBRARYDIR:STRING=$(BOOST_ROOT)/lib

build: wiped kdl

configured: $(SOURCE_DIR) Makefile
	-cd $(SOURCE_DIR) && patch -p0 <../../$(PATCH1)
	-cd $(SOURCE_DIR) && patch -p0 <../../$(PATCH2)
	-mkdir -p $(SOURCE_DIR)/build
	cd $(SOURCE_DIR)/build && $(CMAKE) $(CMAKE_ARGS) ..
	touch configured

wiped: Makefile
	make wipe
	touch wiped

kdl:	configured
	#cd $(SOURCE_DIR)/build && make $(PARALLEL_JOBS) && make python && make install
	cd $(SOURCE_DIR)/build && make $(PARALLEL_JOBS) && make install

clean:
	rm -rf $(SOURCE_DIR)
	rm -rf configured

wipe: 	clean
	rm -rf $(TARBALL) build
