cmake_minimum_required(VERSION 3.0)
project(sylar)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/yaml-cpp)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/jsoncpp-1.8.4)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/sqlite-3.28.0)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/tinyxml2-7.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations -Wno-class-memaccess")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
#set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS}")

ENABLE_LANGUAGE(ASM)

include_directories(.)
include_directories(/apps/sylar/include)
include_directories(/apps/sylar/tair/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/yaml-cpp/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/jsoncpp-1.8.4/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/sqlite-3.28.0)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/tinyxml2-7.0.1)
link_directories(/usr/lib64)
link_directories(/apps/sylar/lib)
link_directories(/apps/sylar/lib64)
link_directories(/apps/sylar/tair/lib)

option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(MySQL REQUIRED)
if(MYSQL_FOUND)
    include_directories(${MYSQL_INCLUDE_DIR})
endif()

find_package(Protobuf)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()

set(LIB_SRC
    sylar/address.cc
    sylar/bytearray.cc
    sylar/config.cc
    sylar/dns.cc
    sylar/db/fox_thread.cc
    sylar/db/mysql.cc
    sylar/db/redis.cc
    sylar/db/tair.cc
    sylar/db/sqlite3.cc
    sylar/ds/bitmap.cc
    sylar/ds/roaring_bitmap.cc
    sylar/ds/roaring.c
    sylar/ds/util.cc
    sylar/email/email.cc
    sylar/email/smtp.cc
    sylar/env.cc
    sylar/daemon.cc
    sylar/fd_manager.cc
    sylar/file_manager.cc
    sylar/fiber.cc
    sylar/fcontext/fcontext.S
    sylar/libco/coctx.cc
    sylar/libco/coctx_swap.S
    sylar/http/http.cc
    sylar/http/http_connection.cc
    sylar/http/http_parser.cc
    sylar/http/http_session.cc
    sylar/http/http_server.cc
    sylar/http/servlet.cc
    sylar/http/servlets/config_servlet.cc
    sylar/http/servlets/status_servlet.cc
    sylar/http/session_data.cc
    sylar/http/ws_connection.cc
    sylar/http/ws_session.cc
    sylar/http/ws_server.cc
    sylar/http/ws_servlet.cc
    sylar/hook.cc
    sylar/iomanager.cc
    sylar/library.cc
    sylar/log.cc
    sylar/module.cc
    sylar/mutex.cc
    sylar/ns/name_server_module.cc
    sylar/ns/ns_client.cc
    sylar/ns/ns_protocol.cc
    sylar/protocol.cc
    sylar/rock/rock_protocol.cc
    sylar/rock/rock_server.cc
    sylar/rock/rock_stream.cc
    sylar/scheduler.cc
    sylar/socket.cc
    sylar/stream.cc
    sylar/streams/async_socket_stream.cc
    sylar/streams/socket_stream.cc
    sylar/streams/load_balance.cc
    sylar/streams/service_discovery.cc
    sylar/streams/zlib_stream.cc
    sylar/tcp_server.cc
    sylar/timer.cc
    sylar/thread.cc
    sylar/util.cc
    sylar/util/crypto_util.cc
    sylar/util/json_util.cc
    sylar/util/hash_util.cc
    sylar/util/trace.cc
    sylar/util/tracker.cc
    sylar/worker.cc
    sylar/application.cc
    sylar/zk_client.cc
    )

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

protobufmaker(sylar/ns/ns_protobuf.proto LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR})
#PROTOBUF_GENERATE_CPP(PB_SRCS PB_HDRS sylar/ns/ns_protobuf.proto)
#message("****${PB_SRCS}***${PB_HDRS}****")
#list(APPEND LIB_SRC ${PB_SRCS})
#
message(STATUS ${LIB_SRC})

add_library(sylar STATIC ${LIB_SRC})
#add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar)
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (sylar_static PROPERTIES OUTPUT_NAME "sylar")

#find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)
#set(LIBS ${LIBS} sylar)
#set(LIBS ${LIBS} dl)
#set(LIBS ${LIBS} ${YAMLCPP})
#set(LIBS ${LIBS} ${PTHREAD})

set(LIBS
        "-Wl,--whole-archive"
        sylar
        "-Wl,--no-whole-archive"
        dl
        pthread
        tbb
        yaml-cpp
        jsoncpp
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        event
        hiredis_vip
        mysqlclient_r
        zookeeper_mt
        sqlite3
        tinyxml2
        jemalloc
        tairclientapi
        tbsys
        snappy
        )
sylar_add_executable(test_util "tests/test_util.cc" sylar "${LIBS}")
sylar_add_executable(test_hashmultimap "tests/test_hashmultimap.cc" sylar "${LIBS}")
sylar_add_executable(test_hashmap "tests/test_hashmap.cc" sylar "${LIBS}")
sylar_add_executable(test_dict "tests/test_dict.cc" sylar "${LIBS}")
sylar_add_executable(test_array "tests/test_array.cc" sylar "${LIBS}")
sylar_add_executable(test_tair "tests/test_tair.cc" sylar "${LIBS}")
if(BUILD_TEST)
sylar_add_executable(test1 "tests/test.cc" sylar "${LIBS}")
sylar_add_executable(test_config "tests/test_config.cc" sylar "${LIBS}")
sylar_add_executable(test_thread "tests/test_thread.cc" sylar "${LIBS}")
sylar_add_executable(test_fiber "tests/test_fiber.cc" sylar "${LIBS}")
sylar_add_executable(test_scheduler "tests/test_scheduler.cc" sylar "${LIBS}")
sylar_add_executable(test_iomanager "tests/test_iomanager.cc" sylar "${LIBS}")
sylar_add_executable(test_hook "tests/test_hook.cc" sylar "${LIBS}")
sylar_add_executable(test_address "tests/test_address.cc" sylar "${LIBS}")
sylar_add_executable(test_socket "tests/test_socket.cc" sylar "${LIBS}")
sylar_add_executable(test_bytearray "tests/test_bytearray.cc" sylar "${LIBS}")
sylar_add_executable(test_http "tests/test_http.cc" sylar "${LIBS}")
sylar_add_executable(test_http_parser "tests/test_http_parser.cc" sylar "${LIBS}")
sylar_add_executable(test_tcp_server "tests/test_tcp_server.cc" sylar "${LIBS}")
sylar_add_executable(echo_server "examples/echo_server.cc" sylar "${LIBS}")
sylar_add_executable(test_http_server "tests/test_http_server.cc" sylar "${LIBS}")
sylar_add_executable(test_uri "tests/test_uri.cc" sylar "${LIBS}")
sylar_add_executable(my_http_server "samples/my_http_server.cc" sylar "${LIBS}")

sylar_add_executable(echo_server_udp "examples/echo_server_udp.cc" sylar "${LIBS}")
sylar_add_executable(echo_udp_client "examples/echo_udp_client.cc" sylar "${LIBS}")

sylar_add_executable(test_daemon "tests/test_daemon.cc" sylar "${LIBS}")
sylar_add_executable(test_env "tests/test_env.cc" sylar "${LIBS}")
sylar_add_executable(test_ws_server "tests/test_ws_server.cc" sylar "${LIBS}")
sylar_add_executable(test_ws_client "tests/test_ws_client.cc" sylar "${LIBS}")
sylar_add_executable(test_application "tests/test_application.cc" sylar "${LIBS}")

sylar_add_executable(test_http_connection "tests/test_http_connection.cc" sylar "${LIBS}")
sylar_add_executable(test_lru "tests/test_lru.cc" sylar "${LIBS}")
sylar_add_executable(test_timed_cache "tests/test_timed_cache.cc" sylar "${LIBS}")
sylar_add_executable(test_timed_lru_cache "tests/test_timed_lru_cache.cc" sylar "${LIBS}")
sylar_add_executable(test_zlib_stream "tests/test_zlib_stream.cc" sylar "${LIBS}")

endif()
sylar_add_executable(test_crypto "tests/test_crypto.cc" sylar "${LIBS}")
sylar_add_executable(test_sqlite3 "tests/test_sqlite3.cc" sylar "${LIBS}")
sylar_add_executable(test_rock "tests/test_rock.cc" sylar "${LIBS}")
sylar_add_executable(test_email  "tests/test_email.cc" sylar "${LIBS}")
sylar_add_executable(test_mysql "tests/test_mysql.cc" sylar "${LIBS}")
sylar_add_executable(test_nameserver "tests/test_nameserver.cc" sylar "${LIBS}")
sylar_add_executable(test_bitmap "tests/test_bitmap.cc" sylar "${LIBS}")
sylar_add_executable(test_zkclient "tests/test_zookeeper.cc" sylar "${LIBS}")
sylar_add_executable(test_service_discovery "tests/test_service_discovery.cc" sylar "${LIBS}")

set(ORM_SRCS
    sylar/orm/table.cc
    sylar/orm/column.cc
    sylar/orm/index.cc
    sylar/orm/orm.cc
    sylar/orm/util.cc
    )
sylar_add_executable(orm "${ORM_SRCS}" sylar "${LIBS}")

add_library(test_module SHARED tests/test_module.cc)

sylar_add_executable(bin_sylar "sylar/main.cc" sylar "${LIBS}")
set_target_properties(bin_sylar PROPERTIES OUTPUT_NAME "sylar")

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/orm_out)
#set(OLIBS ${LIBS} orm_data)
#sylar_add_executable(test_orm "tests/test_orm.cc" orm_data "${OLIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
