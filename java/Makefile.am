# OLA Java client

EXTRA_DIST = pom.xml \
             src/main/java/ola/OlaClient.java \
             src/main/java/ola/rpc/SimpleRpcController.java \
             src/main/java/ola/rpc/StreamRpcChannel.java \
             src/test/java/ola/OlaClientTest.java \
             src/test/java/ola/OlaClientUtilsTest.java

generated-src/main/java/ola/proto/Ola.java: ${top_srcdir}/common/protocol/Ola.proto
	mkdir -p $(top_builddir)/java/generated-src/main/java
	$(PROTOC) --java_out=$(top_builddir)/java/generated-src/main/java --proto_path=${top_srcdir}/common/protocol ${top_srcdir}/common/protocol/Ola.proto

generated-src/main/java/ola/rpc/Rpc.java: ${top_srcdir}/common/rpc/Rpc.proto
	mkdir -p $(top_builddir)/java/generated-src/main/java
	$(PROTOC) --java_out=$(top_builddir)/java/generated-src/main/java --proto_path=${top_srcdir}/common/rpc ${top_srcdir}/common/rpc/Rpc.proto

ola.jar: generated-src/main/java/ola/proto/Ola.java generated-src/main/java/ola/rpc/Rpc.java
	cd ${top_builddir}/java && mvn --file ${top_srcdir}/java/pom.xml -DbuildDir=${abs_top_builddir}/java -DprotobufGeneratedSource=$(abs_top_builddir)/java/generated-src/main/java -DprotobufJavaVersion=$(PROTOBUF_VERSION) package

CLEANFILES = generated-src/main/java/ola/proto/Ola.java \
             generated-src/main/java/ola/rpc/Rpc.java \
             classes/ola/proto/*.class \
             classes/ola/rpc/*.class \
             classes/ola/*.class \
             test-classes/ola/*.class \
             maven-archiver/pom.properties \
             surefire-reports/ola.*.txt \
             surefire-reports/TEST-*.xml \
             ola-java-client-*.jar

if BUILD_JAVA_LIBS
# .jars aren't really data, but it sure ain't a program.
noinst_DATA = ola.jar
endif
