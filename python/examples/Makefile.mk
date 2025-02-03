# example python scripts
dist_noinst_SCRIPTS += \
    python/examples/ola_artnet_params.py \
    python/examples/ola_candidate_ports.py \
    python/examples/ola_devices.py \
    python/examples/ola_fetch_dmx.py \
    python/examples/ola_patch_unpatch.py \
    python/examples/ola_plugin_info.py \
    python/examples/ola_rdm_discover.py \
    python/examples/ola_rdm_get.py \
    python/examples/ola_recv_dmx.py \
    python/examples/ola_send_dmx.py \
    python/examples/ola_simple_fade.py \
    python/examples/ola_universe_info.py \
    python/examples/rdm_compare.py \
    python/examples/rdm_snapshot.py

CLEANFILES += \
    python/examples/*.pyc \
    python/examples/__pycache__/*
