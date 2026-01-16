cmake_minimum_required(VERSION 3.1)

project(QuickStart)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(BYTERTC_LINUX 1)
    message("system name: ${CMAKE_SYSTEM_NAME} info:${CMAKE_SYSTEM_PROCESSOR}")
    IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
        set(SYSTEM_X86 1)
    ELSE(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
        set(SYSTEM_X86 0)
    ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    IF(CMAKE_CL_64)
        set(BYTERTC_WIN32 0)
    ELSE(CMAKE_CL_64)
        set(BYTERTC_WIN32 1)
    ENDIF(CMAKE_CL_64)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
else()
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")



set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#set ouput path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/$(Configuration)/QuickStart)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/$(Configuration)/QuickStart)
message(STATUS "EXECUTABLE_OUTPUT_PATH = ****${PROJECT_BINARY_DIR}")

# 新目录结构路径
set(BYTERTC_SDK_DIR "${CMAKE_SOURCE_DIR}/3rdparty/VolcEngineRTC_arm")
set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
set(CONFIG_FILE "${CMAKE_SOURCE_DIR}/config.json")


find_package(Qt5 COMPONENTS Widgets Core Gui Network OpenGL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)

# GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

if (SYSTEM_X86)
    set(VolcEngineRTC_Lib "3rdparty/VolcEngineRTC_x86")
else(SYSTEM_X86)
    set(VolcEngineRTC_Lib "3rdparty/VolcEngineRTC_arm")
endif(SYSTEM_X86)

#link VolcEngineRTC lib
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/${VolcEngineRTC_Lib}/include)
include_directories(${CMAKE_SOURCE_DIR}/${VolcEngineRTC_Lib}/include/rtc)
include_directories(${CMAKE_SOURCE_DIR}/${VolcEngineRTC_Lib}/include/game)
include_directories(${GSTREAMER_INCLUDE_DIRS})

# 新目录结构 include 路径
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/app)
include_directories(${CMAKE_SOURCE_DIR}/src/ui/widgets)
include_directories(${CMAKE_SOURCE_DIR}/src/ui/components)
include_directories(${CMAKE_SOURCE_DIR}/src/core/api)
include_directories(${CMAKE_SOURCE_DIR}/src/core/rtc)
include_directories(${CMAKE_SOURCE_DIR}/src/core/media)
include_directories(${CMAKE_SOURCE_DIR}/src/core/config)
include_directories(${CMAKE_SOURCE_DIR}/src/drivers/interfaces)
include_directories(${CMAKE_SOURCE_DIR}/src/drivers/impl/linux)
include_directories(${CMAKE_SOURCE_DIR}/src/common)
include_directories(${CMAKE_SOURCE_DIR}/src/common/TokenGenerator)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#ui - 新路径
FILE(GLOB UI_FILES "src/ui/widgets/*.ui")
qt5_wrap_ui(MainWindow_UI_FILES ${UI_FILES})
list(APPEND ALL_SOURCES_AND_HEADERS ${UI_FILES})

message(STATUS "**************${MainWindow_UI_FILES}*************")
qt5_add_resources(MainWindow_QRC_FILES resources/app.qrc)
#qrc

#sources - 新目录结构
FILE(GLOB_RECURSE SRC_APP "src/app/*.h" "src/app/*.cpp")
FILE(GLOB_RECURSE SRC_UI_WIDGETS "src/ui/widgets/*.h" "src/ui/widgets/*.cpp")
FILE(GLOB_RECURSE SRC_UI_COMPONENTS "src/ui/components/*.h" "src/ui/components/*.cpp")
FILE(GLOB_RECURSE SRC_CORE_API "src/core/api/*.h" "src/core/api/*.cpp")
FILE(GLOB_RECURSE SRC_CORE_RTC "src/core/rtc/*.h" "src/core/rtc/*.cpp")
FILE(GLOB_RECURSE SRC_CORE_MEDIA "src/core/media/*.h" "src/core/media/*.cpp")
FILE(GLOB_RECURSE SRC_CORE_CONFIG "src/core/config/*.h" "src/core/config/*.cpp")
FILE(GLOB_RECURSE SRC_DRIVERS_INTERFACES "src/drivers/interfaces/*.h")
FILE(GLOB_RECURSE SRC_DRIVERS "src/drivers/impl/linux/*.h" "src/drivers/impl/linux/*.cpp")
FILE(GLOB_RECURSE SRC_COMMON "src/common/*.h" "src/common/*.cpp")
FILE(GLOB SRC_MAIN "src/main.cpp")

source_group(app FILES ${SRC_APP})
source_group(ui/widgets FILES ${SRC_UI_WIDGETS})
source_group(ui/components FILES ${SRC_UI_COMPONENTS})
source_group(core/api FILES ${SRC_CORE_API})
source_group(core/rtc FILES ${SRC_CORE_RTC})
source_group(core/media FILES ${SRC_CORE_MEDIA})
source_group(core/config FILES ${SRC_CORE_CONFIG})
source_group(drivers/interfaces FILES ${SRC_DRIVERS_INTERFACES})
source_group(drivers FILES ${SRC_DRIVERS})
source_group(common FILES ${SRC_COMMON})

list(APPEND ALL_SOURCES_AND_HEADERS 
    ${SRC_MAIN}
    ${SRC_APP}
    ${SRC_UI_WIDGETS}
    ${SRC_UI_COMPONENTS}
    ${SRC_CORE_API}
    ${SRC_CORE_RTC}
    ${SRC_CORE_MEDIA}
    ${SRC_CORE_CONFIG}
    ${SRC_DRIVERS_INTERFACES}
    ${SRC_DRIVERS}
    ${SRC_COMMON}
)

add_executable(QuickStart
        ${ALL_SOURCES_AND_HEADERS}
        ${MainWindow_QRC_FILES}
        "resources/app.rc"
        )

IF (BYTERTC_LINUX)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/${VolcEngineRTC_Lib}/lib/)
set(CMAKE_PREFIX_PATH $ENV{QTDIR}/lib/cmake) #don't forget to set env path QTDIR
target_link_options(${PROJECT_NAME} PUBLIC -Wl,-rpath-link=${BYTERTC_SDK_DIR}/lib/libVolcEngineRTC.so)
set(CMAKE_CXX_FLAGS "-ggdb '-std=c++14' -fPIC -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
ENDIF ()

message('so'=${BYTERTC_SDK_DIR}/lib/libVolcEngineRTC.so)

find_path(PULSEAUDIO_INCLUDE_DIR
        NAMES pulse/pulseaudio.h
        DOC "The PulseAudio include directory"
    )
find_library(PULSEAUDIO_LIBRARY
        NAMES pulse
        DOC "The PulseAudio library"
    )
target_include_directories(${PROJECT_NAME} PUBLIC ${PULSEAUDIO_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC
        Qt5::Widgets
        Qt5::Network
        Qt5::OpenGL
        OpenGL::GL
        # RTCFFmpeg
        VolcEngineRTC
        pulse-simple pulse
        atomic
        OpenSSL::Crypto
        ${GSTREAMER_LIBRARIES}
        )

set(DST_DIR \"${PROJECT_BINARY_DIR}\")
set(LIB_DIR \"${CMAKE_SOURCE_DIR}/3rdparty/VolcEngineRTC_arm/lib\")
set(ARCHIVE_DIR archive)


add_custom_target(
  archive
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ARCHIVE_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy  ${LIB_DIR}/*.so ${ARCHIVE_DIR}/
  COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}  ${ARCHIVE_DIR}/
  COMMAND patchelf --set-rpath '$$ORIGIN' ${CMAKE_BINARY_DIR}/${ARCHIVE_DIR}/${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_SOURCE_DIR}/resources/default.desktop  ${ARCHIVE_DIR}/default.desktop
  COMMAND linuxdeployqt ${CMAKE_BINARY_DIR}/${ARCHIVE_DIR}/${PROJECT_NAME} -always-overwrite -appimage
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_dependencies(archive ${PROJECT_NAME})

add_custom_command(
  TARGET archive POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E tar cvf "${ARCHIVE_DIR}.zip" --format=zip
  ${ARCHIVE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
