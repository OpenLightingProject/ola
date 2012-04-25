SUBDIRS = network io rdm thread web messaging testing timecode

SOURCES = ActionQueue.h BaseTypes.h Callback.h CallbackRunner.h Clock.h \
          DmxBuffer.h ExportMap.h Logging.h MultiCallback.h \
          RunLengthEncoder.h StringUtils.h

BUILT_SOURCES = plugin_id.h

EXTRA_DIST = $(SOURCES) gen_callbacks.py make_plugin_id.sh
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)

plugin_id.h: make_plugin_id.sh
	sh ./make_plugin_id.sh

clean-local:
	rm -f plugin_id.h
