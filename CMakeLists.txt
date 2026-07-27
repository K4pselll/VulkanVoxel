cmake_minimum_required(VERSION 3.20)
project(VulkanVoxel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Fetch GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

add_executable(voxel src/main.cpp)

target_include_directories(voxel PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(voxel glfw Vulkan::Vulkan glm::glm)

# Shader compilation
find_program(GLSLC glslc HINTS "$ENV{VULKAN_SDK}/Bin")

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADERS
    ${SHADER_DIR}/shader.vert
    ${SHADER_DIR}/shader.frag
    ${SHADER_DIR}/shader2d.vert
    ${SHADER_DIR}/shader2d.frag
)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
        OUTPUT ${SHADER_OUT_DIR}/${SHADER_NAME}.spv
        COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUT_DIR}/${SHADER_NAME}.spv
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPIRV_OUTPUTS ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(voxel shaders)
