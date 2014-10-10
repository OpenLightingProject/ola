# Programs
#########################
if BUILD_OLA_PROTOC
noinst_PROGRAMS += protoc/ola_protoc

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

else

# If we're using a different ola_protoc, we need to provide a rule to create
# this file since the generated service configs depend on it.
protoc/ola_protoc$(EXEEXT):
	touch protoc/ola_protoc$(EXEEXT)

endif
