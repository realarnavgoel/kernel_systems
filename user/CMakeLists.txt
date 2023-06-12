include_directories(${CMAKE_SOURCE_DIR}/../common)
add_compile_options(-g -O3 -Werror -Wall)
add_executable(TestKDriver app.c)

