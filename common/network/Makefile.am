include $(top_srcdir)/common.mk

EXTRA_DIST = PosixInterfacePicker.h SocketHelper.h WindowsInterfacePicker.h

noinst_LTLIBRARIES = libolanetwork.la
libolanetwork_la_SOURCES = AdvancedTCPConnector.cpp \
                           HealthCheckedConnection.cpp \
                           IPV4Address.cpp \
                           Interface.cpp \
                           InterfacePicker.cpp \
                           NetworkUtils.cpp \
                           MACAddress.cpp \
                           Socket.cpp \
                           SocketAddress.cpp \
                           SocketHelper.cpp \
                           TCPConnector.cpp \
                           TCPSocket.cpp

if USING_WIN32
libolanetwork_la_SOURCES += WindowsInterfacePicker.cpp
else
libolanetwork_la_SOURCES += PosixInterfacePicker.cpp
endif

