cmake_minimum_required(VERSION 3.20)
project(web_software_design_five)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)
## 一定要在使用 qt5_add_resources前调用find_package.因为你需要先导入Qt5的库后才会有qt5_add_resources这个函数。
## qt5_add_resources这个函数是在qt5的CMakeLists.txt的库中。所以以后还是在开头导入库为好
set(QRC_FILE src/link.qrc)
qt5_add_resources(QRC ${QRC_FILE})

add_executable(Server src/Server/qt_server_dialog.cpp src/Server/qt_server_dialog.h src/Server/qt_server_dialog.ui
        qt_server_dialog_main.cpp src/common_definition.h src/Server/server_socket_handle.cpp src/Server/server_socket_handle.h)
add_executable(Client src/Client/qt_client_dialog.cpp src/Client/qt_client_dialog.h src/Client/qt_client_dialog.ui
        qt_client_dialog_main.cpp src/common_definition.h src/Client/client_socket_handle.cpp src/Client/client_socket_handle.h
        src/Client/qt_client_file_handle_dialog.cpp src/Client/qt_client_file_handle_dialog.h src/Client/qt_client_file_handle_dialog.ui
        ${QRC})

target_link_libraries(Server
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )
target_link_libraries(Client
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        )

