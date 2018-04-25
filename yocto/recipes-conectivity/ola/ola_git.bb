SUMMARY = "Open Lighting Architecture - OLA"
DESCRIPTION = "The Open Lighting Architecture (OLA) is part of the Open Lighting Project and provides applications with a mechanism to send and receive DMX512 & RDM commands using hardware devices and DMX over IP protocols. This enables software lighting controllers to communicate with hardware either via Ethernet or traditional DMX512 networks. \
OLA can also convert DMX512 data sent using DMX over IP protocols from one format to another, allowing devices from different manufacturers to interact with one another. For example a Strand Lighting Console using ShowNet can send DMX512 to an Enttec EtherGate. When combined with a physical DMX interface such as the DMX USB Pro, OLA can send and receive data from wired DMX512 networks."

LICENSE = "GPLv2 & LGPLv2.1"
LIC_FILES_CHKSUM = "file://LICENCE;md5=7aa5f01584d845ad733abfa9f5cad2a1"

#DEPENDS_class-nativesdk = "bison libusb1 protobuf protobuf-native pkgconfig-native ossp-uuid ola-native "
DEPENDS_class-target    = "bison libmicrohttpd avahi libusb1 libftdi cppunit protobuf protobuf-native ola-native"
DEPENDS_class-native    = "protobuf bison-native flex-native pkgconfig-native ossp-uuid-native"

PV = "0.10.6"

# Esta revision de git tiene un parche para protobuf, la tag de 0.10.6 no compila.
SRCREV = "00dc86a48ec4c528cec90166435b440f283a9c86" 
SRC_URI = "git://github.com/OpenLightingProject/ola.git;protocol=https"

S = "${WORKDIR}/git"
B = "${WORKDIR}/git"

inherit autotools-brokensep relative_symlinks
# autotools-brokensep 
# forces in-source bulding (ola has bugs for out-of-tree builds). 
# See: https://www.yoctoproject.org/docs/latest/ref-manual/ref-manual.html#migration-1.7-autotools-class-changes
# 
# relative_symlinks
# Absolute symbolic links (symlinks) within staged files are no longer permitted and now trigger an error.
# inherit relative_symlinks within the recipe to turn those absolute symlinks into relative
# https://www.yoctoproject.org/docs/2.4/ref-manual/ref-manual.html#migration-2.3-absolute-symlinks

#EXTRA_OECONF = " --disable-unittests --disable-rdm-tests --disable-python-libs --disable-java-libs --disable-all-plugins"
EXTRA_OECONF  = " --disable-unittests --disable-rdm-tests --disable-java-libs "

# nativesdk is for the toolchain, only libola and ola_common are required, all plugins are dynamic
EXTRA_OECONF_append_class-nativesdk = " --with-ola-protoc-plugin=${STAGING_BINDIR_NATIVE}/ola_protoc_plugin \
                                        --disable-all-plugins --disable-e133 --disable-python-libs --disable-examples --disable-http"
      

# native    is for the strange ola_protoc_plugin required at build time
EXTRA_OECONF_append_class-native    = " --disable-all-plugins --disable-e133 --disable-python-libs --disable-examples --disable-http"

# target    are the real config options
EXTRA_OECONF_append_class-target = " --with-ola-protoc-plugin=${STAGING_BINDIR_NATIVE}/ola_protoc_plugin --disable-python-libs\
  --enable-http     --enable-dummy            --enable-gpio   --enable-opendmx --enable-e131 \
  --enable-opendmx  --enable-usbdmx           --enable-usbpro --enable-spi     --enable-spidmx \
  --enable-artnet   --enable-openpixelcontrol --enable-renard --enable-kinet \
  --enable-nanoleaf --enable-karate           --enable-espnet --enable-milinst --enable-pathport \
  --enable-sandnet  --enable-shownet          --enable-stageprofi \
  --enable-e133 \
  --enable-examples "

# -fvisibility-inlines-hidden breaks stuff
CXXFLAGS = "${CFLAGS}"

# The code is not Werror safe
do_configure_prepend() {
  sed -i -e 's:-Werror::g' ${S}/configure.ac
  sed -i -e 's:-Werror::g' ${S}/Makefile.am
}

do_install_append_class-native() {
  install -d ${D}${bindir}
  install -m 0755 ${B}/protoc/ola_protoc_plugin ${D}${bindir}
}

FILES_${PN}-dbg += "${libdir}/*/.debug"
FILES_${PN} += "${datadir}/olad ${libdir}/olad/*.so.*"
FILES_${PN}-staticdev += "${libdir}/olad/*.a"
FILES_${PN}-dev += "${libdir}/olad/*.la ${libdir}/olad/*.so"

RDEPENDS_${PN}-dev = ""

BBCLASSEXTEND = "native nativesdk"