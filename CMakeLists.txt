cmake_minimum_required(VERSION 3.15)
project(libGfx LANGUAGES CXX)

# ================ 基础配置 ================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# ================ 依赖配置 ================
# 查找 SDL2
find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    message(STATUS "Found SDL2: ${SDL2_VERSION}")
else()
    message(FATAL_ERROR "SDL2 not found!")
endif()

# 查找 SDL2_image
find_package(SDL2_image REQUIRED)
if(SDL2_image_FOUND)
    message(STATUS "Found SDL2_image")
else()
    message(FATAL_ERROR "SDL2_image not found!")
endif()

# 查找 SDL2_ttf
find_package(SDL2_ttf REQUIRED)
if(SDL2_ttf_FOUND)
    message(STATUS "Found SDL2_ttf")
else()
    message(FATAL_ERROR "SDL2_ttf not found!")
endif()

# 查找 OpenGL
find_package(OpenGL REQUIRED)
if(OpenGL_FOUND)
    message(STATUS "Found OpenGL")
else()
    message(FATAL_ERROR "OpenGL not found!")
endif()

# 查找 GLEW
find_package(GLEW REQUIRED)
if(GLEW_FOUND)
    message(STATUS "Found GLEW")
else()
    message(FATAL_ERROR "GLEW not found!")
endif()

# 查找 GLM
find_package(glm REQUIRED)
if(glm_FOUND)
    message(STATUS "Found GLM")
else()
    message(FATAL_ERROR "GLM not found!")
endif()

# 查找 FreeType
find_package(Freetype REQUIRED)
if(Freetype_FOUND)
    message(STATUS "Found FreeType: ${Freetype_VERSION}")
else()
    message(FATAL_ERROR "FreeType not found!")
endif()

# ================ 目标配置 ================
# 主库
add_library(libGfx STATIC 
    src/libGfx.cpp
    src/libGfxEvent.cpp
    src/FontA.cpp
    # 添加其他源文件...
)

target_include_directories(libGfx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}  # 添加 FreeType 头文件路径
)

target_link_libraries(libGfx PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${FREETYPE_LIBRARIES}  # 添加 FreeType 库
)

# ================ 安装配置 ================
install(TARGETS libGfx
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# ================ 示例程序 ================
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_executable(example0 examples/example0.cpp)
    target_link_libraries(example0 PRIVATE libGfx)

    # 可以添加更多示例...
endif()
