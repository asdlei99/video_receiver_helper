cmake_minimum_required(VERSION 3.7)

set(LIBWEBRTC_INCLUDE_PATH "D:/webrtcBuild/Sept/webrtc_checkout/src")
set(ABSL_PATH "${LIBWEBRTC_INCLUDE_PATH}/third_party/abseil-cpp")


if(MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

add_library(media_render_util
	video_render_util.h
	video_render_util.cpp
	)

target_include_directories(media_render_util PUBLIC 
	$<BUILD_INTERFACE:${ABSL_PATH}>
	"./../."
	"./../../"
	"./../../json_parser/include/"
#	"./../../websocket_client/"
#	"./../../json_parser"
	)
