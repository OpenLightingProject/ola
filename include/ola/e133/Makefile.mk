olae133includedir = $(pkgincludedir)/e133/

if INSTALL_E133
olae133include_HEADERS = \
    include/ola/e133/DeviceManager.h \
    include/ola/e133/E133Enums.h \
    include/ola/e133/E133Helper.h \
    include/ola/e133/E133Receiver.h \
    include/ola/e133/E133StatusHelper.h \
    include/ola/e133/E133URLParser.h \
    include/ola/e133/MessageBuilder.h
endif
