cmake_minimum_required(VERSION 3.16)

project(QTget VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql Network)

# 启用自动处理moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 创建可执行文件
add_executable(QTget
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    DatabaseManager.cpp
    DatabaseManager.h
    HistoryModel.cpp
    HistoryModel.h
    NetworkManager.h
    NetworkManager.cpp
)

# 链接Qt库
target_link_libraries(QTget PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
)

# 设置包含目录
target_include_directories(QTget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 删除这部分的Windows特定设置，或者改为下面的正确格式
# if(WIN32)
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
# endif()

# 可选的安装规则
install(TARGETS QTget
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
