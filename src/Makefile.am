
AM_CFLAGS = -Wall -Werror $(liblla_CFLAGS)
AM_LDFLAGS =  -LLIBDIR $(liblla_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = lla_dmxconsole lla_dmxmonitor
lla_dmxconsole_SOURCES = lla-dmxconsole.c
lla_dmxconsole_LDADD = @LIBS@ -llla -lcurses
lla_dmxmonitor_SOURCES = lla-dmxmonitor.c
lla_dmxmonitor_LDADD = @LIBS@ -llla -lcurses
endif 

bin_PROGRAMS = lla_patch lla_dev_info lla_plugin_info lla_uni_info lla_uni_name lla_uni_data lla_latency $(NCURSES_PROGS)

lla_patch_SOURCES = lla-patch.c
lla_dev_info_SOURCES = lla-dev-info.c
lla_plugin_info_SOURCES = lla-plugin-info.c
lla_uni_info_SOURCES = lla-uni-info.c
lla_uni_name_SOURCES = lla-uni-name.c
lla_uni_data_SOURCES = lla-uni-data.c
lla_latency_SOURCES = lla-latency.c
