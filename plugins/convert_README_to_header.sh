#!/bin/sh

# A simple script to build a C++ header file containing the plugin description
# from the plugin's README.md
# The output file then contains one variable 'plugin_description'.

if [ $# != 2 ]; then
  echo "Usage: convert_README_to_header.sh <path> <destination_filename>";
  echo "<destination_filename>: e.g. PluginNameDescription.h";
  exit;
fi

path="$1";
outfile="$path/$2";

if [ ! -d $path ]; then
  echo "directory '$path' does not exist";
  exit;
fi

if [ ! -e "$path/README.md" ]; then
  echo "README.md file in '$path' does not exist";
  exit;
fi

plugin=`basename "$path"`

desc=`sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/\\\\\\\\n"\n"/g' "$path/README.md"`;

identifier="PLUGINS_${plugin}_DESCRIPTION_H_";
identifier=`echo "$identifier" | tr '[:lower:]' '[:upper:]'`

echo "#ifndef $identifier" > $outfile;
echo "#define $identifier\n" >> $outfile;
echo "char[] plugin_description = \"$desc\";\n" >> $outfile;
echo "#endif  // $identifier" >> $outfile;