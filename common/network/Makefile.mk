# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/network/AdvancedTCPConnector.cpp \
    common/network/FakeInterfacePicker.h \
    common/network/HealthCheckedConnection.cpp \
    common/network/IPV4Address.cpp \
    common/network/IPV6Address.cpp \
    common/network/Interface.cpp \
    common/network/InterfacePicker.cpp \
    common/network/MACAddress.cpp \
    common/network/NetworkUtils.cpp \
    common/network/NetworkUtilsInternal.h \
    common/network/Socket.cpp \
    common/network/SocketAddress.cpp \
    common/network/SocketCloser.cpp \
    common/network/SocketHelper.cpp \
    common/network/SocketHelper.h \
    common/network/TCPConnector.cpp \
    common/network/TCPSocket.cpp

common_libolacommon_la_LIBADD += $(RESOLV_LIBS)

if USING_WIN32
common_libolacommon_la_SOURCES += \
    common/network/WindowsInterfacePicker.h \
    common/network/WindowsInterfacePicker.cpp
else
common_libolacommon_la_SOURCES += \
    common/network/PosixInterfacePicker.h \
    common/network/PosixInterfacePicker.cpp
endif

# TESTS
##################################################
test_programs += \
    common/network/HealthCheckedConnectionTester \
    common/network/NetworkTester \
    common/network/TCPConnectorTester

common_network_HealthCheckedConnectionTester_SOURCES = \
    common/network/HealthCheckedConnectionTest.cpp
common_network_HealthCheckedConnectionTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_network_HealthCheckedConnectionTester_LDADD = $(COMMON_TESTING_LIBS)

common_network_NetworkTester_SOURCES = \
    common/network/IPV4AddressTest.cpp \
    common/network/IPV6AddressTest.cpp \
    common/network/InterfacePickerTest.cpp \
    common/network/InterfaceTest.cpp \
    common/network/MACAddressTest.cpp \
    common/network/NetworkUtilsTest.cpp \
    common/network/SocketAddressTest.cpp \
    common/network/SocketTest.cpp
common_network_NetworkTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_network_NetworkTester_LDADD = $(COMMON_TESTING_LIBS)

if USING_WIN32
common_network_NetworkTester_LDFLAGS = -no-undefined -liphlpapi -lnetapi32 \
                                       -lcap -lws2_32 -ldpnet -lwsock32
endif

common_network_TCPConnectorTester_SOURCES = \
    common/network/AdvancedTCPConnectorTest.cpp \
    common/network/TCPConnectorTest.cpp
common_network_TCPConnectorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_network_TCPConnectorTester_LDADD = $(COMMON_TESTING_LIBS)
