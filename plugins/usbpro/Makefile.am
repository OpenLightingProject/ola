
libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

BUILT_SOURCES = ConfigMessages.pb.cc ConfigMessages.pb.h

libllausbpro_la_SOURCES = UsbProPlugin.cpp UsbProDevice.cpp\
                          UsbProWidget.cpp UsbProPort.cpp \
                          ConfigMessages.pb.cc

EXTRA_DIST = ConfigMessages.proto UsbProWidgetListener.h UsbProWidget.h \
             UsbProPlugin.h UsbProDevice.h UsbProPort.h

ConfigMessages.pb.cc ConfigMessages.pb.h: ConfigMessages.proto
	protoc --cpp_out ./ ConfigMessages.proto

clean-local:
	rm *.pb.{h,cc}
