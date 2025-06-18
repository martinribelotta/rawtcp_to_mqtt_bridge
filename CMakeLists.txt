cmake_minimum_required(VERSION 3.18)
project(tcp_mqtt_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# CPM.cmake for dependency management
FetchContent_Declare(
  CPM
  GIT_REPOSITORY https://github.com/cpm-cmake/CPM.cmake.git
  GIT_TAG origin/master
)
FetchContent_MakeAvailable(CPM)

# Dependencies
CPMAddPackage("gh:fmtlib/fmt#11.2.0")               # libfmt
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.15.3
  OPTIONS "SPDLOG_FMT_EXTERNAL 1" 
)
CPMAddPackage("gh:jbeder/yaml-cpp#0.8.0")           # YAML parser
CPMAddPackage("gh:nlohmann/json@3.11.3")            # JSON parser (optional)
CPMAddPackage("gh:jeremy-rifkin/cpptrace@0.8.3")    # Stack trace library (cpptrace)
CPMAddPackage("gh:nlohmann/json@3.12.0")            # JSON parser (nlohmann_json)

add_library(nlohmann INTERFACE IMPORTED)

SET(INJA_USE_EMBEDDED_JSON OFF)

CPMAddPackage("gh:pantor/inja#v3.4.0")              # Templating engine (inja)

set(Boost_NO_SYSTEM_PATHS ON) # Importante: fuerza a ignorar Boost del sistema

CPMAddPackage(
  NAME boost
  GITHUB_REPOSITORY boostorg/boost
  GIT_TAG boost-1.84.0
  OPTIONS
    "BOOST_ENABLE_CMAKE ON"
    "BOOST_INCLUDE_LIBRARIES=core;asio;system;assert;config;program_options"
)

CPMAddPackage(
  NAME boost_mqtt5
  GITHUB_REPOSITORY boostorg/mqtt5
  GIT_TAG boost-1.88.0
  OPTIONS "BOOST_ENABLE_CMAKE ON"
)

add_library(Boost::boost INTERFACE IMPORTED)
target_include_directories(Boost::boost INTERFACE "${Boost_SOURCE_DIR}")

add_executable(tcp_mqtt_bridge
    src/main.cpp
    src/slip.cpp
    src/config.cpp
    src/server_manager.cpp
    src/connection_manager.cpp
    src/packet_parser.cpp
    src/packet_parser_yaml.cpp
    src/packet_processor.cpp
    src/mqtt_client.cpp
)

target_link_libraries(tcp_mqtt_bridge
    PRIVATE
        fmt::fmt
        spdlog::spdlog
        yaml-cpp
        nlohmann_json::nlohmann_json
        Boost::asio
        Boost::system
        Boost::core
        Boost::program_options
        Boost::mqtt5
        pantor::inja
        cpptrace::cpptrace
        )

target_include_directories(tcp_mqtt_bridge PRIVATE src)