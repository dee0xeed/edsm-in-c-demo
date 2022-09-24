
TOPDIR := $(shell /bin/pwd)
INCDIR := $(TOPDIR)/include

CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
#CFLAGS += -Wall -I$(INCDIR) -g -D_FILE_OFFSET_BITS=64
CFLAGS += -Wall -Os -I$(INCDIR) -D_FILE_OFFSET_BITS=64

export CC STRIP CFLAGS TOPDIR INCDIR

all:
	@$(MAKE) -C utils
	@$(MAKE) -C utils-tests

	@$(MAKE) -C engine

	@$(MAKE) -C test
	@$(MAKE) -C echo-server

clean:
	@rm -v `find . -name *.o`

cleanall:
	@rm -vf `find . -name *.o`
	@rm -vf `find . -name *.so`
	@rm -vf *.deb

	@$(MAKE) cleanall -C utils-tests
	@$(MAKE) cleanall -C test
	@$(MAKE) cleanall -C echo-server
