SUBDIRS = base dmx network file http io rdm thread web math messaging slp \
          system acn e133 stl testing timecode util client

EXTRA_DIST = gen_callbacks.py make_plugin_id.sh

pkginclude_HEADERS = ActionQueue.h BaseTypes.h Callback.h CallbackRunner.h \
                     Clock.h Constants.h DmxBuffer.h ExportMap.h Logging.h \
                     MultiCallback.h StringUtils.h
nodist_pkginclude_HEADERS = plugin_id.h

plugin_id.h: Makefile make_plugin_id.sh $(top_srcdir)/common/protocol/Ola.proto
	sh $(srcdir)/make_plugin_id.sh $(top_srcdir)/common/protocol/Ola.proto

CLEANFILES = plugin_id.h
