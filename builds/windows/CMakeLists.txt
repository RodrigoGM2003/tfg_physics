## --------------------------------------------------------------------------------
## Informática Gráfica, curso 23-24, ejemplo de OpenGL mínimo.
##
## Archivo de configuración para compilar usando CMake en Windows,
## Asume que las librerías necesarias se han instalado con 'vcpkg'
## última modificación: 25 de Junio del 2023
##
## --------------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.5.1)
set( CMAKE_TOOLCHAIN_FILE ~/vcpkg/scripts/buildsystems/vcpkg.cmake)  ## permite a 'find_package' encontrar librerías instaladas con VCPKG
project( opengl3_minimo )
## ----------------------------------------------------------------------------------------------------
## Aspectos configurables

set( carpeta_fuentes       "../../src" )
set( carpeta_ejecutable    "../bin" )
set( nombre_ejecutable     "ejecutable" )
set( opcs_warnings         "/wd4244 /wd4305 /wd4267" ) ## deshabilitar warnings de conversión implicita double --> float, size_t --> int (este ultimo es dudoso..)
set( opcs_primer_error     "" ) ## "-Wfatal-errors"  --> no hay nada equivalente a esto en el compilador de microsoft, no se puede parar con el 1er error
set( flags_compilador      "/std:c++20  /MT /O2 ${opcs_warnings} ${opcs_primer_error}" ) 
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL) ## necesario para usar algunas funcionalidades de GLM

## ----------------------------------------------------------------------------------------------------
##  definir flags para compilador y carpeta(s) de includes en todos los targets

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flags_compilador}")
include_directories( ${carpeta_fuentes}/* )

## ----------------------------------------------------------------------------------------------------
## buscar librerías en el sistema y añadirlas a todos los targets
## (no se busca GLM ya que es una librería 'header only')

find_package( GLEW REQUIRED )
find_package( glfw3 CONFIG REQUIRED )
link_libraries( GLEW::GLEW glfw )

## ----------------------------------------------------------------------------------------------------
## Add ImGui source files
set(IMGUI_DIR ${carpeta_fuentes}/vendor/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

## ----------------------------------------------------------------------------------------------------
## definir ejecutable (unidades y cabeceras a compilar), indicar carpeta donde debe alojarse el .exe

file(GLOB unidades 
    ${carpeta_fuentes}/*.cpp 
    ${carpeta_fuentes}/meshes/*.cpp
    ${carpeta_fuentes}/buffers/*.cpp
    ${carpeta_fuentes}/tests/*.cpp
    ${carpeta_fuentes}/simulators/*.cpp
)
file(GLOB cabeceras 
    ${carpeta_fuentes}/*.h
    ${carpeta_fuentes}/meshes/*.h
    ${carpeta_fuentes}/buffers/*.h
    ${carpeta_fuentes}/tests/*.h
    ${carpeta_fuentes}/simulators/*.h
)
include_directories(
    ${carpeta_fuentes} 
    ${carpeta_fuentes}/buffers 
    ${carpeta_fuentes}/meshes
    ${carpeta_fuentes}/simulators
)

add_executable(${nombre_ejecutable} ${unidades} ${cabeceras} ${IMGUI_SOURCES} ${TESTS_SOURCES})
set_target_properties(${nombre_ejecutable} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${carpeta_ejecutable})