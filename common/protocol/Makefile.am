

noinst_LTLIBRARIES = libllaproto.la
nodist_libllaproto_la_SOURCES = Lla.pb.cc
EXTRA_DIST = Lla.proto

BUILT_SOURCES = Lla.pb.cc Lla.pb.h

Lla.pb.cc Lla.pb.h: Lla.proto
	protoc --cpp_out ./ Lla.proto

clean-local:
	rm -f *.pb.{h,cc}
