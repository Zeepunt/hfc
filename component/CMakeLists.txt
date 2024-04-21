# for MbedTLS
option(ENABLE_TESTING "Build mbed TLS tests." OFF)
option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." ON)

# for nghttp2
option(ENABLE_LIB_ONLY "Build libnghttp2 only." ON)

add_subdirectory(mbedtls-3.4.0)
add_subdirectory(nghttp2-1.53.0)