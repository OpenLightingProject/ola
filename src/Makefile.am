
AM_CFLAGS = -Wall -Werror 
AM_LDFLAGS =  -LLIBDIR

bin_PROGRAMS = test lla_dmxconsole lla_patch lla_dev_info lla_plugin_info

test_SOURCES = test.c
test_LDADD = @LIBS@ -llla

lla_dmxconsole_SOURCES = lla-dmxconsole.c
lla_dmxconsole_LDADD = @LIBS@ -llla -lcurses

lla_patch_SOURCES = lla-patch.c
lla_patch_LDADD = @LIBS@ -llla 

lla_dev_info_SOURCES = lla-dev-info.c
lla_dev_info_LDADD = @LIBS@ -llla 

lla_plugin_info_SOURCES = lla-plugin-info.c
lla_plugin_info_LDADD = @LIBS@ -llla 
