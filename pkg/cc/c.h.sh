#!/bin/sh

# Instead of a broken sed script, this uses cproto.

ODIN_FILE=$1;shift; ODIN_dir=$1;shift; ODIN_incsp=$1;shift;
ODIN_define=$1;shift;

flags=
if [ "$ODIN_define" != "" ] ; then
   for def in `cat $ODIN_define`; do
      flags="$flags -D$def"; done; fi
flags="$flags -I$ODIN_dir"
if [ "$ODIN_incsp" != "" ] ; then
   for sp in `cat $ODIN_incsp`; do
      flags="$flags -I$sp"; done; fi

if [ "$ODINVERBOSE" != "" ] ; then
   echo ${ODINRBSHOST}cproto $flags `basename $ODIN_FILE`; fi

# the following simulates the existing Func.hh files:
# -f2 removes parameter names (types only)
# -e adds "extern" to each prototype
# -q silently fails if include files are missing
cproto $flags -q -f2 -e $ODIN_FILE >c.h 2>WARNINGS \
 || { mv WARNINGS ERRORS; echo "cproto failed" >>ERRORS; }

exit 0
