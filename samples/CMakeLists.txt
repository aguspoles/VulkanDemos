cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(Samples LANGUAGES C CXX)

include_directories(INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(ENGINE_FILES
    # Header Files
    DemoApplication.h
    # Source Files
    main.cpp
    DemoApplication.cpp
)

set(PLATFORM_FILES
    # Header Files
    platform/GlfwWindow.h
    platform/Platform.h
    platform/Window.h
    platform/WindowsPlatform.h
    platform/FileSystem.h
    platform/Application.h
    platform/InputEvents.h
    # Source Files
    platform/GlfwWindow.cpp
    platform/Platform.cpp
    platform/Window.cpp
    platform/WindowsPlatform.cpp
    platform/FileSystem.cpp
    platform/Application.cpp
    platform/InputEvents.cpp
    )

set(CORE_FILES
    # Header Files
    core/Timer.h
    core/Logger.h
    core/Error.h
    core/Helpers.h
    # Source Files
    core/Timer.cpp
    core/Logger.cpp
    core/Error.cpp
    core/Helpers.cpp
    core/glm_defs.h
)

set(RENDER_FILES
    # Header Files
    render/VulkanRenderer.h
    render/Utilities.h
    render/GraphicsPipeline.h
    render/PipelineState.h
    render/Swapchain.h
    render/CommandPool.h
    render/CommandBuffer.h
    render/Mesh.h
    # Source Files
    render/VulkanRenderer.cpp
    render/Utilities.cpp
    render/GraphicsPipeline.cpp
    render/PipelineState.cpp
    render/Swapchain.cpp
    render/CommandPool.cpp
    render/CommandBuffer.cpp
    render/Mesh.cpp
)

set(SCENE_FILES 
    scene/Camera.h
    scene/GameObject.h
    scene/Camera.cpp
    scene/GameObject.cpp
    )

set(LOADER_FILES
    # Header Files
    # Source Files
)

source_group("\\" FILES ${ENGINE_FILES})
source_group("core\\" FILES ${CORE_FILES})
source_group("platform\\" FILES ${PLATFORM_FILES})
source_group("render\\" FILES ${RENDER_FILES})
source_group("loaders\\" FILES ${LOADER_FILES})
source_group("scene\\" FILES ${SCENE_FILES})

set(PROJECT_FILES
    ${ENGINE_FILES}
    ${PLATFORM_FILES}
    ${CORE_FILES}
    ${RENDER_FILES}
    ${LOADER_FILES}
    ${SCENE_FILES}
    )

# Add files based on platform
if(DIRECT_TO_DISPLAY)
    list(APPEND PROJECT_FILES ${LINUX_D2D_FILES})
    message(STATUS "Unix d2d platform detected")
else()
    list(APPEND PROJECT_FILES ${GLFW_FILES})
    if(WIN32)
        list(APPEND PROJECT_FILES ${WINDOWS_FILES})
        message(STATUS "Windows platform detected")
    else()
        list(APPEND PROJECT_FILES ${UNIX_FILES})
        message(STATUS "Unix platform detected")
    endif()
endif()

# Pre compiled headers
pch_pch(PROJECT_FILES pch.cpp)

add_executable(${PROJECT_NAME} WIN32 ${PROJECT_FILES})

# compiler flags based on compiler type
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC -fexceptions)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /MP)
endif()

if(${VKB_VALIDATION_LAYERS})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS)
endif()

# GPU assisted validation layers are not available on macOS.
if(${VKB_VALIDATION_LAYERS_GPU_ASSISTED})
    if (APPLE)
        message(WARNING "GPU assisted validation layers are not currently available on macOS.")
    else()
        target_compile_definitions(${PROJECT_NAME} PUBLIC VKB_VALIDATION_LAYERS_GPU_ASSISTED)
    endif()
endif()

if(${VKB_WARNINGS_AS_ERRORS})
    message(STATUS "Warnings as Errors Enabled")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif()
endif()

# Link third party libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    glm
    vulkan
    spdlog
    tinyobj
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

target_include_directories(${PROJECT_NAME} PUBLIC PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})


# Create MSVC project
if(MSVC)
    #Set the working directory to the source of the project so developer dont have to
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    #Configure output paths
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} SUFFIX)
        string(TOLOWER ${CONFIG_TYPE} CONFIG_DIR)
        set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
        set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/lib/${CONFIG_DIR}/${TARGET_ARCH})
    endforeach()
endif()