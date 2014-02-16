# OLA Java client

EXTRA_DIST = pom.xml \
             src/main/java/ola/OlaClient.java \
             src/main/java/ola/rpc/SimpleRpcController.java \
             src/main/java/ola/rpc/StreamRpcChannel.java

src/main/java/ola/proto/Ola.java: ${top_srcdir}/common/protocol/Ola.proto
	$(PROTOC) --java_out=src/main/java --proto_path=${top_srcdir}/common/protocol ${top_srcdir}/common/protocol/Ola.proto

src/main/java/ola/rpc/Rpc.java: ${top_srcdir}/common/rpc/Rpc.proto
	$(PROTOC) --java_out=src/main/java --proto_path=${top_srcdir}/common/rpc ${top_srcdir}/common/rpc/Rpc.proto

ola.jar: src/main/java/ola/proto/Ola.java src/main/java/ola/rpc/Rpc.java
	mvn package
ola_jar_SOURCES =

CLEANFILES = src/main/java/ola/proto/Ola.java \
             src/main/java/ola/rpc/Rpc.java

if BUILD_JAVA_LIBS
# hardly a program...
noinst_PROGRAMS = ola.jar
endif
