#!/bin/bash -x
#
# Fetch the new PIDs definition files from the RDM PID Index
datahost="rdm.openlighting.org"
if [ ! -z $1 ]; then
	datahost=$1
fi

echo "Fetching PID data from $datahost"

curl -o pids.proto -f http://$datahost/download?pids=esta
curl -o draft_pids.proto -f http://$datahost/download?pids=esta-draft
curl -o manufacturer_pids.proto -f http://$datahost/download?pids=manufacturers
curl -o manufacturer_names.proto -f http://$datahost/download?pids=manufacturer-names
