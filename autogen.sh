#!/bin/sh

aclocal
#libtool
autoconf
autoheader
automake --add-missing
