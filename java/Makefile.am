# ola Java client

EXTRA_DIST = pom.xml

if BUILD_JAVA_LIBS

BUILT_SOURCES = src/main/java/ola/proto/Ola.java \
                src/main/java/ola/rpc/Rpc.java

# hardly a program...
noinst_PROGRAMS = ola.jar

ola.jar: src/main/java/ola/proto/Ola.java src/main/java/ola/rpc/Rpc.java
	mvn package

src/main/java/ola/proto/Ola.java: ../common/protocol/Ola.proto
	protoc --java_out=src/main/java --proto_path=../common/protocol ../common/protocol/Ola.proto

src/main/java/ola/rpc/Rpc.java: ../common/rpc/Rpc.proto
	protoc --java_out=src/main/java --proto_path=../common/rpc ../common/rpc/Rpc.proto

clean-local:
	rm -f src/main/java/ola/proto/Ola.java
	rm -f src/main/java/ola/rpc/Rpc.java

endif
