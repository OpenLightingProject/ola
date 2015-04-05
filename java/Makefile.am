# OLA Java client

EXTRA_DIST = pom.xml \
             src/main/java/ola/OlaClient.java \
             src/main/java/ola/rpc/SimpleRpcController.java \
             src/main/java/ola/rpc/StreamRpcChannel.java \
             src/test/java/ola/OlaClientTest.java

src/main/java/ola/proto/Ola.java: ${top_srcdir}/common/protocol/Ola.proto
	mkdir -p $(top_builddir)/java/src/main/java
	$(PROTOC) --java_out=$(top_builddir)/java/src/main/java --proto_path=${top_srcdir}/common/protocol ${top_srcdir}/common/protocol/Ola.proto

src/main/java/ola/rpc/Rpc.java: ${top_srcdir}/common/rpc/Rpc.proto
	mkdir -p $(top_builddir)/java/src/main/java
	$(PROTOC) --java_out=$(top_builddir)/java/src/main/java --proto_path=${top_srcdir}/common/rpc ${top_srcdir}/common/rpc/Rpc.proto

ola.jar: src/main/java/ola/proto/Ola.java src/main/java/ola/rpc/Rpc.java
	cat ${top_builddir}/java/pom.xml
	ls -al ${top_srcdir}/java
	ls -al ${abs_top_srcdir}/java
	ls -al ${top_builddir}/java
	ls -al ${abs_top_builddir}/java
	cd ${top_srcdir}/java && mvn package
	ls -al ${top_srcdir}/java
	ls -al ${top_builddir}/java

CLEANFILES = src/main/java/ola/proto/Ola.java \
             src/main/java/ola/rpc/Rpc.java

if BUILD_JAVA_LIBS
# .jars aren't really data, but it sure ain't a program.
noinst_DATA = ola.jar
endif
