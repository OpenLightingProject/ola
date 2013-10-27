#!/bin/bash
#
# Fetch the new PIDs definition files from the RDM PID Index
wget -O pids.proto http://rdm.openlighting.org/download?pids=esta
wget -O draft_pids.proto http://rdm.openlighting.org/download?pids=esta-draft
wget -O manufacturer_pids.proto http://rdm.openlighting.org/download?pids=manufacturers
