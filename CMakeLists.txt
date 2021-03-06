cmake_minimum_required(VERSION 3.19)

project(raycaster VERSION 0.0.1
                 DESCRIPTION "Raycaster"
                 LANGUAGES C CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(GLM REQUIRED)

# Will be replaced eventually, however when files are being created frequently during development, this is a lot easier
file(GLOB SOURCE_FILES
          CONFIGURE_DEPENDS
          src/*.cpp
)

file(GLOB HEADER_FILES
          CONFIGURE_DEPENDS
          src/*.hpp
)

add_executable(raycaster ${HEADER_FILES} ${SOURCE_FILES})

set(GLAD_INCLUDE_DIR "dep/glad/include")

add_library(GLAD STATIC dep/glad/src/glad.c)
target_include_directories(GLAD PUBLIC ${GLAD_INCLUDE_DIR})

target_include_directories(raycaster PRIVATE ${SDL2_INCLUDE_DIR})
target_include_directories(raycaster PRIVATE ${SDL2_IMAGE_INCLUDE_DIR})

target_link_libraries(raycaster PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(raycaster PRIVATE GLAD)
target_link_libraries(raycaster PRIVATE ${GLM_LIBRARIES})
target_link_libraries(raycaster PRIVATE ${SDL2_LIBRARY})
target_link_libraries(raycaster PRIVATE ${SDL2_IMAGE_LIBRARIES})
target_link_libraries(raycaster PRIVATE ${CMAKE_DL_LIBS})
