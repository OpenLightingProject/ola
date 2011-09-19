SUBDIRS = network rdm thread web messaging

SOURCES = ActionQueue.h BaseTypes.h Callback.h Clock.h DmxBuffer.h \
          ExportMap.h Logging.h OlaThread.h RunLengthEncoder.h StringUtils.h

BUILT_SOURCES = plugin_id.h

EXTRA_DIST = $(SOURCES) gen_callbacks.py make_plugin_id.sh
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)

plugin_id.h:
	sh ./make_plugin_id.sh

clean-local:
	rm -f plugin_id.h
