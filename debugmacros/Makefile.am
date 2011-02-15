noinst_LIBRARIES=libdebugmacros.a

# Only program to build here
libdebugmacros_a_SOURCES=debugmacros.cc debugmacros.h

# Copy the configuration files
CONFIG_FILES=debugmacros.conf

%.conf: %.conf.bak
	cp -f $< $@

.PHONY: all
all: $(CONFIG_FILES)
