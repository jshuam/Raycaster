cmake_minimum_required(VERSION 3.16.3)

project(raycaster VERSION 0.0.1
                 DESCRIPTION "Raycaster"
                 LANGUAGES C CXX
)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(sdl2-image CONFIG REQUIRED)
find_package(sdl2-mixer CONFIG REQUIRED)
find_package(sdl2-ttf CONFIG REQUIRED)

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

target_link_libraries(raycaster PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(raycaster PRIVATE glad::glad)
target_link_libraries(raycaster PRIVATE glm::glm)
target_link_libraries(raycaster PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_link_libraries(raycaster PRIVATE SDL2::SDL2_image)
target_link_libraries(raycaster PRIVATE SDL2::SDL2_mixer)
target_link_libraries(raycaster PRIVATE SDL2::SDL2_ttf)
