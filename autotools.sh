#!/bin/sh

# This script calls the autotools to generate the build infrastructure
# After this was done, the developer can use './configure' and 'make'

aclocal    && \
autoconf   && \
autoheader && \
automake   && \
echo -e "Autotools completed successfully.\nUse \"./configure\" and \"make\" to build Chronos!"
