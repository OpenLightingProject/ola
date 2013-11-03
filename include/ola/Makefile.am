SUBDIRS = base dmx network file http io rdm thread web math messaging slp acn \
          e133 stl testing timecode util api

SOURCES = ActionQueue.h BaseTypes.h Callback.h CallbackRunner.h Clock.h \
          Constants.h DmxBuffer.h ExportMap.h Logging.h MultiCallback.h \
          StringUtils.h

BUILT_SOURCES = plugin_id.h

EXTRA_DIST = $(SOURCES) gen_callbacks.py make_plugin_id.sh
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)

plugin_id.h: make_plugin_id.sh ../../common/protocol/Ola.proto
	sh ./make_plugin_id.sh

clean-local:
	rm -f plugin_id.h
