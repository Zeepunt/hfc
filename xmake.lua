set_project("httpc")
set_xmakever("2.7.8")
set_version("0.0.1", {build = "%Y%m%d%H%M"})

set_languages("gnu99", "gnuxx11")
-- add_cflags("-g", "-DDEBUG")
add_cflags("-Os")

includes("component")

target("httpc")
    set_kind("static")
    add_deps("mbedtls")
    add_deps("nghttp2")
    add_includedirs("src", {public = true})
    add_files("src/*.c")

target("get_example")
    set_kind("binary")
    add_deps("httpc")
    add_files("example/get_example.c")

target("post_example")
    set_kind("binary")
    add_deps("httpc")
    add_files("example/post_example.c")

target("httpc2_example")
    set_kind("binary")
    add_deps("httpc")
    add_files("example/httpc2_example.c")
