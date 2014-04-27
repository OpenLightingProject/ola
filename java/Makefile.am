# OLA Java client

EXTRA_DIST = pom.xml \
             src/main/java/ola/OlaClient.java \
             src/main/java/ola/rpc/SimpleRpcController.java \
             src/main/java/ola/rpc/StreamRpcChannel.java \
             src/test/java/ola/OlaClientTest.java

src/main/java/ola/proto/Ola.java: ${top_srcdir}/common/protocol/Ola.proto
	$(PROTOC) --java_out=src/main/java --proto_path=${top_srcdir}/common/protocol ${top_srcdir}/common/protocol/Ola.proto

src/main/java/ola/rpc/Rpc.java: ${top_srcdir}/common/rpc/Rpc.proto
	$(PROTOC) --java_out=src/main/java --proto_path=${top_srcdir}/common/rpc ${top_srcdir}/common/rpc/Rpc.proto

ola.jar: src/main/java/ola/proto/Ola.java src/main/java/ola/rpc/Rpc.java
	mvn package

CLEANFILES = src/main/java/ola/proto/Ola.java \
             src/main/java/ola/rpc/Rpc.java

if BUILD_JAVA_LIBS
# .jars aren't really data, but it sure ain't a program.
noinst_DATA = ola.jar
endif
