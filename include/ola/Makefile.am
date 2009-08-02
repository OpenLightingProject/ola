
SUBDIRS = network .

SOURCES = BaseTypes.h ExportMap.h StringUtils.h \
          Closure.h Logging.h DmxBuffer.h

BUILT_SOURCES = plugin_id.h

EXTRA_DIST = $(SOURCES) make_plugin_id.sh
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)

plugin_id.h:
	./make_plugin_id.sh

clean-local:
	rm -f plugin_id.h
