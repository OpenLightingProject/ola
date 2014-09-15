#!/bin/bash

VERSION_FILE="include/ola/client/Version.h"

major=`grep OLA_CLIENT_VERSION_MAJOR $VERSION_FILE | cut -d\  -f3`
minor=`grep OLA_CLIENT_VERSION_MINOR $VERSION_FILE | cut -d\  -f3`
revision=`grep OLA_CLIENT_VERSION_REVISION $VERSION_FILE | cut -d\  -f3`

echo "$major.$minor.$revision"
