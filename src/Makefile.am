
AM_CFLAGS = -Wall -Werror 
AM_LDFLAGS =  -LLIBDIR

bin_PROGRAMS = test lla_dmxconsole lla_patch

test_SOURCES = test.c
test_LDADD = @LIBS@ -llla

lla_dmxconsole_SOURCES = lla-dmxconsole.c
lla_dmxconsole_LDADD = @LIBS@ -llla -lcurses

lla_patch_SOURCES = lla-patch.c
lla_patch_LDADD = @LIBS@ -llla -lcurses
