#!/bin/bash

XML_SOURCES="usbpro artnet"

for src in $XML_SOURCES; do
	file="../plugins/$src/ConfMessages.xml"

	if [ ! -f $file ]; then
		echo Could not find $src;
		exit 1;
	fi

	./generate.pl $file;

	# fix me
	cp lla-$src/* ../plugins/$src/conf_msgs/
done
