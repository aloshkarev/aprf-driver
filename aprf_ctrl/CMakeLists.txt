cmake_minimum_required(VERSION 3.10)
project(aprf-ctrl)

set(DEB_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(DEB_PACKAGE_VERSION "1.0.1")
set(DEB_PACKAGE_SECTION "misc")
set(DEB_PACKAGE_DESRCIPTION "Control for aprf-drv module")
set(DEB_PACKAGE_MAINTAINER "Aleksandr Loshkarev <hi@aloshkarev.ru>")
set(DEB_PACKAGE_ARCH "amd64")
set(DEB_PACKAGE_DEPENDS "libnl-3-dev, libnl-genl-3-dev, libevent-dev")

# find libraries
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/deb_packaging.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindNetLink.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibEvent.cmake)
find_package(Threads)
# include libnl directories
include_directories(${NL_INCLUDE_DIRS})
add_definitions(-m64)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m64")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m64")
# set source files
set(SOURCE_FILES
        hwsim_ctrl/hwsim_ctrl_cli.c
        hwsim_ctrl/hwsim_ctrl_cli.h
        hwsim_ctrl/hwsim_ctrl_func.c
        hwsim_ctrl/hwsim_ctrl_func.h
        hwsim_ctrl/hwsim_ctrl_event.c
        hwsim_ctrl/hwsim_ctrl_event.h)

# add executables
add_executable(aprf_ctrl ${SOURCE_FILES})

# link required libraries
target_link_libraries(aprf_ctrl m)
target_link_libraries(aprf_ctrl nl-3 nl-genl-3)
target_link_libraries(aprf_ctrl event)
target_link_libraries(aprf_ctrl ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aprf_ctrl DESTINATION /usr/local/bin/)