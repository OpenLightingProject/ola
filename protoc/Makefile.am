include $(top_srcdir)/common.mk

EXTRA_DIST = CppGenerator.h CppFileGenerator.h GeneratorHelpers.h \
             ServiceGenerator.h StrUtil.h

# Programs
#########################
noinst_PROGRAMS = ola_protoc

ola_protoc_SOURCES = ola-protoc.cpp \
                     CppGenerator.cpp \
                     CppFileGenerator.cpp \
                     GeneratorHelpers.cpp \
                     ServiceGenerator.cpp \
                     StrUtil.cpp
ola_protoc_LDADD = $(libprotobuf_LIBS) \
                   -lprotoc
