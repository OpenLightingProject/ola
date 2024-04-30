noinst_LTLIBRARIES += libs/sniffer/libolasniffer.la

libs_sniffer_libolasniffer_la_SOURCES = \
    libs/sniffer/BaseSnifferReader.cpp \
    libs/sniffer/BaseSnifferReader.h \
    libs/sniffer/DMXSignalProcessor.cpp \
    libs/sniffer/DMXSignalProcessor.h
libs_sniffer_libolasniffer_la_CXXFLAGS = $(COMMON_CXXFLAGS)
libs_sniffer_libolasniffer_la_LIBADD = common/libolacommon.la
