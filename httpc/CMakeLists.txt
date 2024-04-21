file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.c src/*.cpp)

add_library(httpc OBJECT ${sources})
target_include_directories(httpc PUBLIC include)
target_link_libraries(httpc PUBLIC mbedtls nghttp2)