# 项目名称
project(RecordCore)
cmake_minimum_required(VERSION 3.5)

if (WIN32)
elseif (UNIX)
	MESSAGE(STATUS "UNIX")

	# CMAKE_CURRENT_SOURCE_DIR：CMakeLists.txt所在目录
	set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

	# 链接Qt静态库/动态库
	# set(LINK_QT_STATIC TRUE)

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#	set(CMAKE_AUTOUIC ON)
#	set(CMAKE_AUTOMOC ON)
#	set(CMAKE_AUTORCC ON)

	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	set(FFMPEG_ROOT_DIR "/home/andy/ffmpeg_build")

	if(${LINK_QT_STATIC})
		set(QT_PATH ${CUR_DIR}/3rd/qt-5.12.9-static)
		message(STATUS "Link with Qt static lib")
	else()
		set(QT_PATH "/home/andy/Qt/6.2.4/gcc_64")
		message(STATUS "Link with Qt shared lib")
	endif()

	set(CMAKE_PREFIX_PATH ${QT_PATH})
	message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
	find_package(QT NAMES Qt6 ${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)
	find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)
	message(STATUS "Qt version: ${QT_VERSION_MAJOR}")

	# 指定头文件目录
	include_directories(
	if(${LINK_QT_STATIC})
	${QT_PATH}/include
	${QT_PATH}/include/QtCore
	${QT_PATH}/include/QtGui
	${QT_PATH}/include/QtWidget
	else()
	endif()
	${FFMPEG_ROOT_DIR}/include
	../common
	../common/log
	../common/util
	./include
	./src
	)

	# 添加源文件
	aux_source_directory(./src SRC_LIST)
	LIST(APPEND SRC_LIST
	../common/util/util.cpp
	)

	# 指定库文件目录
	link_directories(
	${QT_PATH}/lib
	${QT_PATH}/plugins/platforms
	${FFMPEG_ROOT_DIR}/lib
	)

	set(EXECUTABLE_OUTPUT_PATH ${CUR_DIR}/bin)
	add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
	set_target_properties(${PROJECT_NAME} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    )
	# message(STATUS "CMAKE_CURRENT_SOURCE_DIR is ${CMAKE_CURRENT_SOURCE_DIR}")

    set(QT_LIBS Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

    set(FFMPEG_LIBS swscale swresample avutil avformat avdevice avcodec)
	# Bsymbolic参考：https://ffmpeg.org/platform.html#Advanced-linking-configuration
	target_link_libraries(${PROJECT_NAME} 
	-Wl,-Bsymbolic,--start-group 
    ${QT_LIBS} ${FFMPEG_LIBS}
	-Wl,-Bsymbolic,--end-group)
endif()

#-O2 -g -DNDEBUG
set(APP_DEFINE "-DQT_MESSAGELOGCONTEXT")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -std=c++11 -Wno-deprecated-declarations -finput-charset=gbk ${APP_DEFINE}")
# set(CMAKE_CXX_FLAGS "-O0 -g -DNDEBUG -std=c++11 -Wno-deprecated-declarations -finput-charset=gbk ${APP_DEFINE}")
set(CMAKE_CXX_FLAGS "-O0 -g -DNDEBUG -std=c++11 -Wno-deprecated-declarations ${APP_DEFINE}")
message(STATUS "cxx flags is ${CMAKE_CXX_FLAGS}")
