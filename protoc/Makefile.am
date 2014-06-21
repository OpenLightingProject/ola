include $(top_srcdir)/common.mk

# Programs
#########################
if BUILD_OLA_PROTOC
noinst_PROGRAMS = ola_protoc
endif

ola_protoc_SOURCES = CppFileGenerator.cpp CppFileGenerator.h CppGenerator.cpp \
                     CppGenerator.h GeneratorHelpers.cpp GeneratorHelpers.h \
                     ServiceGenerator.cpp ServiceGenerator.h StrUtil.cpp \
                     StrUtil.h ola-protoc.cpp
ola_protoc_LDADD = $(libprotobuf_LIBS) \
                   -lprotoc
