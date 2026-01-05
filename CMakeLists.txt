cmake_minimum_required(VERSION 3.10)
project(AimTrainer)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(AimTrainer src/main.cpp)

target_link_libraries(AimTrainer PRIVATE 
    glfw 
    GLEW::GLEW 
    glm::glm
    opengl32
)