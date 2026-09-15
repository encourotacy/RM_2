cmake_minimum_required(VERSION 3.16.3)

project(class_3)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(include)

add_executable(main main.cpp)
add_executable(show_img show_img.cpp)
add_executable(main_hw main_hw.cpp)
add_executable(show_img_hw show_img_hw.cpp)

target_link_libraries(main ${OpenCV_LIBS} )
target_link_libraries(show_img ${OpenCV_LIBS} )
target_link_libraries(main_hw ${OpenCV_LIBS} )
target_link_libraries(show_img_hw ${OpenCV_LIBS} )
