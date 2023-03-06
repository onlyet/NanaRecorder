# 项目名称
project(NaNaRecorder)
cmake_minimum_required(VERSION 3.5)

if (WIN32)
elseif (UNIX)
	MESSAGE(STATUS "UNIX")

	# CMAKE_CURRENT_SOURCE_DIR：CMakeLists.txt所在目录
	set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

	# 链接Qt静态库/动态库
	# set(LINK_QT_STATIC TRUE)

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    set(CMAKE_AUTORCC ON)
	set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)

	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    ../RecordCore/include
	../common
	../common/log
	../common/util
    ../common/dump
    ./model
	)

#    file(GLOB recorder ./*.h ./*.ui ./*.cpp ./model/*)
#    source_group("Recorder" FILES ${Recorder})

#    file(GLOB interface ../RecordCore/include/*.h)
#    source_group("Interface" FILES ${Interface})

#    file(GLOB common ../common/util/* ../common/dump/*)
#    source_group("common" FILES ${common})

	# 添加源文件
    aux_source_directory(./ SRC_LIST)
    LIST(APPEND SRC_LIST
    ./model/AppData.cpp
    ../common/util/util.cpp
    ../common/dump/dump.cpp
    )

	set(FFMPEG_ROOT_DIR "/home/andy/ffmpeg_build")

	# 指定库文件目录
	link_directories(
	${QT_PATH}/lib
	${QT_PATH}/plugins/platforms
    ${CMAKE_CURRENT_SOURCE_DIR}/../RecordCore/lib
	${FFMPEG_ROOT_DIR}/lib
	)

	set(EXECUTABLE_OUTPUT_PATH ${CUR_DIR}/../bin)
    set(QRC_PATH ${CUR_DIR}/../res/NanaRecorder.qrc)
    add_executable(${PROJECT_NAME} ${SRC_LIST} #[[${common}]] ${QRC_PATH} #[[${Recorder}]] #[[${Interface}]])

    set(QT_LIBS Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

    set(FFMPEG_LIBS swscale swresample avutil avformat avdevice avcodec)

	target_link_libraries(${PROJECT_NAME} 
	-Wl,--start-group 
    RecordCore ${QT_LIBS} ${FFMPEG_LIBS}
	-Wl,--end-group)
endif()

set(APP_DEFINE "-DQT_MESSAGELOGCONTEXT")
set(CMAKE_CXX_FLAGS "-O0 -g -DNDEBUG -std=c++11 -Wno-deprecated-declarations ${APP_DEFINE}")
message(STATUS "cxx flags is ${CMAKE_CXX_FLAGS}")
