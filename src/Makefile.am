
AM_CXXFLAGS = -Wall -Werror $(liblla_CFLAGS)
AM_LDFLAGS =  -LLIBDIR $(liblla_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = lla_dmxconsole lla_dmxmonitor
lla_dmxconsole_SOURCES = lla-dmxconsole.cpp
lla_dmxconsole_LDADD = @LIBS@ -lcurses
lla_dmxmonitor_SOURCES = lla-dmxmonitor.cpp
lla_dmxmonitor_LDADD = @LIBS@ -lcurses
endif 

bin_PROGRAMS = lla_patch lla_dev_info lla_plugin_info lla_uni_info lla_uni_name lla_uni_merge lla_set_dmx lla_usbpro $(NCURSES_PROGS)
lla_patch_SOURCES = lla-patch.cpp
lla_dev_info_SOURCES = lla-client.cpp
lla_plugin_info_SOURCES = lla-client.cpp
lla_uni_info_SOURCES = lla-client.cpp
lla_uni_name_SOURCES = lla-client.cpp
lla_uni_merge_SOURCES = lla-client.cpp
lla_set_dmx_SOURCES = lla-client.cpp
#lla_latency_SOURCES = lla-latency.c
#lla_latency2_SOURCES = lla-latency2.c
lla_usbpro_SOURCES = lla-usbpro.cpp
lla_usbpro_LDADD = @LIBS@ -lllausbproconf
