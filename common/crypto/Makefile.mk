# LIBRARIES
##################################################
if HAVE_LIBCRYPTO
noinst_LTLIBRARIES += common/crypto/libolacrypto.la
common_crypto_libolacrypto_la_SOURCES = \
    common/crypto/HMAC.cpp
common_crypto_libolacrypto_la_LIBADD = $(libcrypto_LIBS)

# TODO(Peter): Fixme for Windows
# if USING_WIN32
# Work around limitations with Windows library linking
# common_crypto_libolacrypto_la_LIBADD = common/libolacommon.la
# endif

# TESTS
################################################
test_programs += common/crypto/HMACTester

COMMON_CRYPTO_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                           common/crypto/libolacrypto.la

common_crypto_HMACTester_SOURCES = \
    common/crypto/HMACTest.cpp
common_crypto_HMACTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_crypto_HMACTester_LDADD = $(COMMON_CRYPTO_TEST_LDADD)
endif

