# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/export_map/ExportMap.cpp

# TESTS
##################################################
test_programs += common/export_map/ExportMapTester

common_export_map_ExportMapTester_SOURCES = common/export_map/ExportMapTest.cpp
common_export_map_ExportMapTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_export_map_ExportMapTester_LDADD = $(COMMON_TESTING_LIBS)
