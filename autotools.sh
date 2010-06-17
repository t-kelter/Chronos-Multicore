#!/bin/sh

# This script calls the autotools to generate the build infrastructure
# After this was done, the developer can use './configure' and 'make'

aclocal                                  && \
autoconf                                 && \
autoheader                               && \
automake                                 && \
echo "Autotools completed successfully." && \
echo "Use \"./configure\" and \"make\" to build Chronos!"
