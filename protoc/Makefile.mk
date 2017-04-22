# Programs
#########################
if BUILD_OLA_PROTOC_PLUGIN
noinst_PROGRAMS += protoc/ola_protoc_plugin

protoc_ola_protoc_plugin_SOURCES = \
    protoc/CppFileGenerator.cpp \
    protoc/CppFileGenerator.h \
    protoc/CppGenerator.cpp \
    protoc/CppGenerator.h \
    protoc/GeneratorHelpers.cpp \
    protoc/GeneratorHelpers.h \
    protoc/ServiceGenerator.cpp \
    protoc/ServiceGenerator.h \
    protoc/StrUtil.cpp \
    protoc/StrUtil.h \
    protoc/ola-protoc-generator-plugin.cpp
protoc_ola_protoc_plugin_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
protoc_ola_protoc_plugin_LDADD = $(libprotobuf_LIBS) -lprotoc

else

# If we're using a different ola_protoc_plugin, we need to provide a rule to
# create this file since the generated service configs depend on it.
protoc/ola_protoc_plugin$(EXEEXT):
	touch protoc/ola_protoc_plugin$(EXEEXT)

endif
