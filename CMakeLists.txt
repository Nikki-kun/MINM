cmake_minimum_required(VERSION 3.16)

project(MINM VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
    Sql
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTRRC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
)

target_include_directories(${PROJECT_NAME} PRIVATE include)