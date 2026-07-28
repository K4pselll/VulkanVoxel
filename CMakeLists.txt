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
find_program(GLSLC glslc
    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
        "${VULKAN_SDK}/Bin"
        "${VULKAN_SDK}/bin"
)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Install Vulkan SDK and ensure glslc is in PATH.")
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADERS
    ${SHADER_SRC_DIR}/shader.vert
    ${SHADER_SRC_DIR}/shader.frag
    ${SHADER_SRC_DIR}/shader2d.vert
    ${SHADER_SRC_DIR}/shader2d.frag
)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_OUT ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_OUT}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV_OUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPIRV_OUTPUTS ${SPV_OUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(voxel shaders)

# Copy SPIR-V shaders next to the executable
add_custom_command(TARGET voxel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUT_DIR}
        $<TARGET_FILE_DIR:voxel>/shaders
    COMMENT "Copying shaders to executable directory"
)
