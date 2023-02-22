# 项目名称
project(NaNaRecorder VERSION 0.1 LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5)

if (WIN32)
elseif (UNIX)
	MESSAGE(STATUS "UNIX")

	# CMAKE_CURRENT_SOURCE_DIR：CMakeLists.txt所在目录
	set(CUR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

	# 链接Qt静态库/动态库
	# set(LINK_QT_STATIC TRUE)

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)

	# 设置自动生成moc文件,AUTOMOC打开可以省去QT5_WRAP_CPP命令
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)

	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	if(${LINK_QT_STATIC})
		set(QT_PATH ${CUR_DIR}/3rd/qt-5.12.9-static)
		message(STATUS "Link with Qt static lib")
	else()
		set(QT_PATH "/opt/qt-5.12.9")
		message(STATUS "Link with Qt shared lib")
	endif()

	set(CMAKE_PREFIX_PATH ${QT_PATH})
	message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
	# find_package(Qt5 COMPONENTS Core Network Concurrent REQUIRED)
	find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
	find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

	# 指定头文件目录
	include_directories(
	if(${LINK_QT_STATIC})
	${QT_PATH}/include
	${QT_PATH}/include/QtCore
	${QT_PATH}/include/QtGui
	${QT_PATH}/include/QtWidget
	else()
	endif()
	./src/app
	./src/common
	./src/common/Http
	)

	# 添加源文件
	aux_source_directory(./src/app SRC_LIST)
	LIST(APPEND SRC_LIST
	./src/common/SingleApplication.cpp
	)

	# 指定库文件目录
	link_directories(
	if(${LINK_QT_STATIC})
	${QT_PATH}/lib
	${QT_PATH}/plugins/platforms
	else()
	${QT_PATH}/plugins/platforms
	${QT_PATH}/lib
	endif()
	)

	set(EXECUTABLE_OUTPUT_PATH ${CUR_DIR}/bin)
	add_executable(${PROJECT_NAME} ${SRC_LIST})

	if(${LINK_QT_STATIC})
	set(QT_LIBS Qt5::Core Qt5::Network Qt5::Concurrent qtpcre2)
	else()
	set(QT_LIBS Qt5::Core Qt5::Network Qt5::Concurrent harfbuzz pcre2-16)
	endif()

	set(LINK_LIBS pthread dl glib-2.0 freetype harfbuzz)
	set(OPENCV_DEPEND_LIBS swscale swresample avutil avformat avfilter avdevice avcodec)
	target_link_libraries(${PROJECT_NAME} -Wl,--start-group 
	${QT_LIBS} ${LINK_LIBS} ${OPENCV_DEPEND_LIBS}
	-Wl,--end-group)
endif ()

#-O2 -g -DNDEBUG
set(APP_DEFINE "-DQT_MESSAGELOGCONTEXT")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -std=c++11 -Wno-deprecated-declarations -finput-charset=gbk ${APP_DEFINE}")
set(CMAKE_CXX_FLAGS "-O0 -g -DNDEBUG -std=c++11 -Wno-deprecated-declarations -finput-charset=gbk ${APP_DEFINE}")
message(STATUS "cxx flags is ${CMAKE_CXX_FLAGS}")
