cmake_minimum_required(VERSION 3.10)
project(CGCC)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ativa o FetchContent
include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4  
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

# Faz o download e compila as bibliotecas
FetchContent_MakeAvailable(glfw glm)

FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb_image)

set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "") # Força compilação estática
set(ASSIMP_BUILD_ZLIB OFF CACHE INTERNAL "") # Evita erro de compilação do zlib no macOS

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.3.1 # Usando uma versão fixa e estável
)
FetchContent_MakeAvailable(assimp)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})
include_directories(${stb_image_SOURCE_DIR})

set(EXERCISES
    # ExAlien_Gabi
    ExCubeMap_Gabi
    Hello3D
    HelloCamera
    HelloCubemap
    HelloCurves
    HelloOBJ_Gabi
    HelloPhong  
    TrabalhoGA_Gabi_Luisa 
    TrabalhoGB_Gabi_Luisa
)

add_compile_options(-Wno-pragmas)

if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

set(GLAD_C_FILE "${CMAKE_SOURCE_DIR}/common/glad.c")

if (NOT EXISTS ${GLAD_C_FILE})
    message(FATAL_ERROR "Arquivo glad.c não encontrado! Baixe a GLAD manualmente e coloque glad.h em include/glad/ e glad.c em common/")
endif()

foreach(EXERCISE ${EXERCISES})
    file(GLOB SOURCES "src/${EXERCISE}/*.cpp" "src/${EXERCISE}/*.h")
    
    if(NOT SOURCES)
        message(WARNING "Nenhum arquivo fonte encontrado para o projeto ${EXERCISE} em src/${EXERCISE}/")
    endif()

    add_executable(${EXERCISE} ${SOURCES} ${GLAD_C_FILE})
    
    target_include_directories(${EXERCISE} PRIVATE ${CMAKE_SOURCE_DIR}/include/glad ${glm_SOURCE_DIR} ${stb_image_SOURCE_DIR})

    target_link_libraries(${EXERCISE} glfw ${OPENGL_LIBS} assimp)
endforeach()