cmake_minimum_required(VERSION 3.1)
# Define uma biblioteca de interface (header-only)
add_library(tiny_obj_loader INTERFACE)

# Define onde os cabeçalhos estão localizados para quem usar esta biblioteca
target_include_directories(tiny_obj_loader INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)