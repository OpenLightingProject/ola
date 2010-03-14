SUBDIRS = network rdm

SOURCES = BaseTypes.h Clock.h Closure.h DmxBuffer.h ExportMap.h Logging.h \
          RunLengthEncoder.h StringUtils.h

BUILT_SOURCES = plugin_id.h

EXTRA_DIST = $(SOURCES) make_plugin_id.sh
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)

plugin_id.h:
	sh ./make_plugin_id.sh

clean-local:
	rm -f plugin_id.h
