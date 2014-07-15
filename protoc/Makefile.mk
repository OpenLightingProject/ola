# Programs
#########################
if BUILD_OLA_PROTOC
noinst_PROGRAMS += protoc/ola_protoc
endif

protoc_ola_protoc_SOURCES = \
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
    protoc/ola-protoc.cpp
protoc_ola_protoc_LDADD = $(libprotobuf_LIBS) -lprotoc
