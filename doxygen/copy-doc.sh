#!/bin/sh

if [ $# != 1 ]; then
  echo "Usage: $0 <output_dir>"
  exit 1;
fi

output_dir=$1
if [ ! -d $output_dir ]; then
  echo $output_dir is not a directory
  exit 1;
fi

echo "Output dir: $output_dir";

if [ ! -d ./html/ ]; then
  echo ./html/ doesn\'t exist, make doxygen-doc failed somehow
  exit 1;
fi

cp -a -v ./html/* $output_dir

chmod -c -R a+r $output_dir

chmod -c a+rx $output_dir/search
