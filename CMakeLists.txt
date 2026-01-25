cmake_minimum_required(VERSION 3.16)

# Configuração de Runtime Estática para MSVC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

project(mygame)

# --- Dependências (Thirdparty) ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/glfw-3.3.2)
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(thirdparty/stb_truetype)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/imgui-docking)
add_subdirectory(thirdparty/tiny_obj_loader)

# --- Coleta de Arquivos Fonte ---
# Agora buscamos o main.cpp e outros fontes diretamente na pasta src
file(GLOB_RECURSE ALL_SOURCES 
    CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
)

# --- Definição do Executável Único ---
# O comando add_executable agora usa a lista automatica de arquivos encontrados
add_executable(${CMAKE_PROJECT_NAME} ${ALL_SOURCES})

# Configurações do Target
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Macro para caminhos de recursos
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC 
    RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/"
)

# Diretórios de Include
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
)

# Linkagem de Bibliotecas
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    glm glfw glad stb_image stb_truetype imgui tiny_obj_loader
)

# Configurações específicas para Windows/MSVC
if(MSVC)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug<$<CONFIG:Debug>:Debug>")
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
endif()