#!/bin/bash

# A simple script to build OLA for mac systems. We need to support multiple
# architectures as well as pre 10.6 systems.

if [ $# != 1 ]; then
  echo "Usage: mac_build.sh <destination_dir>";
  exit;
fi

dest_dir=$1;

if [ ! -d $dest_dir ]; then
  echo "destination_dir ${dest_dir} does not exist";
  exit;
fi

./configure --disable-dependency-tracking --enable-python-libs --disable-slp
make  CPPFLAGS="-arch ppc -arch i386 -arch x86_64 -mmacosx-version-min=10.5"  \
  LDFLAGS=" -arch ppc -arch i386 -arch x86_64 -mmacosx-version-min=10.5"
make check
sudo make install
DESTDIR=${dest_dir} make install
