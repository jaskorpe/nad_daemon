#!/bin/sh
autoheader \
&& aclocal \
&& automake --add-missing \
&& autoconf