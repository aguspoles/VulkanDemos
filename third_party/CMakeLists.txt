cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(third_party LANGUAGES C CXX)

#tinyobj
add_library(tinyobj INTERFACE)
set(TINYOBJ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinyobj)
target_sources(tinyobj INTERFACE ${TINYOBJ_DIR}/tiny_obj_loader.h)
target_include_directories(tinyobj INTERFACE ${TINYOBJ_DIR})

# glm
add_library(glm INTERFACE)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
target_include_directories(glm INTERFACE ${GLM_DIR})

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(glm INTERFACE GLM_FORCE_CXX14)
endif()

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.hpp)
target_include_directories(vulkan INTERFACE ${VULKAN_INCLUDE_DIR})

target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)

if(WIN32)
    target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
	target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_METAL_EXT)
elseif(UNIX)
    # Choose WSI based on VKB_WSI_SELECTION
    if (VKB_WSI_SELECTION STREQUAL XCB OR VKB_WSI_SELECTION STREQUAL XLIB OR VKB_WSI_SELECTION STREQUAL WAYLAND)
        find_package(PkgConfig REQUIRED)
    endif()
    if (VKB_WSI_SELECTION STREQUAL XCB)
        pkg_check_modules(XCB xcb REQUIRED)
        if (XCB_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XCB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL XLIB)
        pkg_check_modules(X11 x11 REQUIRED)
        if (X11_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
        pkg_check_modules(WAYLAND wayland-client REQUIRED)
        if (WAYLAND_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WAYLAND_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL D2D)
        set(DIRECT_TO_DISPLAY TRUE)
        set(DIRECT_TO_DISPLAY TRUE PARENT_SCOPE)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message(FATAL_ERROR "Unknown WSI")
    endif()
endif() 

# GLFW
 if (NOT DIRECT_TO_DISPLAY)
	option(GLFW_BUILD_DOCS OFF)
	option(GLFW_BUILD_TESTS OFF)
	option(GLFW_BUILD_EXAMPLES OFF)
	option(GLFW_BUILD_TESTS OFF)
	option(GLFW_INSTALL OFF)
	option(BUILD_SHARED_LIBS ON)
	if (VKB_WSI_SELECTION STREQUAL WAYLAND)
		set(GLFW_USE_WAYLAND ON)
	endif()
	
	add_subdirectory(glfw)

	if (TARGET update_mappings)
        set_target_properties(update_mappings PROPERTIES FOLDER "ThirdParty")
    endif()
    set_property(TARGET glfw PROPERTY FOLDER "ThirdParty")
endif()

# fmt
#add_subdirectory(fmt)
#set_property(TARGET fmt PROPERTY FOLDER "ThirdParty")

# SPDLOG
#set(SPDLOG_FMT_EXTERNAL ON)
#add_subdirectory(spdlog)
#set_property(TARGET spdlog PROPERTY FOLDER "ThirdParty")

##Remove when fixing spdlog fmt dependency issue
add_library(spdlog INTERFACE)
set(SPDLOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spdlog)
target_include_directories(spdlog INTERFACE ${SPDLOG_DIR}/include)
target_sources(spdlog INTERFACE ${SPDLOG_DIR}/src)
##

