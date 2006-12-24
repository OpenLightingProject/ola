#!/bin/bash

XML_SOURCES="../plugins/usbpro/UsbProConfMessages.xml"

for src in $XML_SOURCES; do
	if [ ! -f $src ]; then
		echo Could not find $src;
		exit 1;
	fi

	./generate.pl $src

	# fix me
	cp lla-UsbPro/* ../plugins/usbpro/conf_msgs/
done
