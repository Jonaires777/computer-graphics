# assignments/task5/CMakeLists.txt

add_executable(task5 main.cpp 
    "${CMAKE_SOURCE_DIR}/src/demoShaderLoader.cpp"
    "${CMAKE_SOURCE_DIR}/src/openglDebug.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Point.h"
    "${CMAKE_SOURCE_DIR}/include/model/Ray.h"
    "../../include/model/PointLight.h"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Sphere.cpp"
    "${CMAKE_SOURCE_DIR}/include/operations/Shading.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Plane.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Cilinder.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Cone.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Triangle.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Objects/Mesh.cpp"
    "${CMAKE_SOURCE_DIR}/include/model/Camera.cpp"
)

set_property(TARGET task5 PROPERTY CXX_STANDARD 17)

target_include_directories(task5 PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(task5 PUBLIC
    RESOURCES_PATH="${CMAKE_SOURCE_DIR}/resources/"
)

target_link_libraries(task5 PRIVATE
    glm glfw glad stb_image stb_truetype imgui Eigen3::Eigen
)


if(MSVC)
    target_compile_definitions(task5 PUBLIC _CRT_SECURE_NO_WARNINGS)
    set_property(TARGET task5 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug<$<CONFIG:Debug>:Debug>")
    set_property(TARGET task5 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
endif()
