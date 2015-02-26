
cmake_minimum_required ( VERSION 2.8.12 )

# toolchain setup
set ( STATICLIB_TOOLCHAIN linux_amd64_clang CACHE STRING "toolchain triplet" )
set ( CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/${STATICLIB_TOOLCHAIN}.cmake CACHE INTERNAL "" )

# project setup
project ( staticlib_cron C )
# common include
include ( ${CMAKE_CURRENT_LIST_DIR}/cmake/CMakeLists_common.cmake )

# build steps
set ( ${PROJECT_NAME}_INCLUDES ${CMAKE_CURRENT_LIST_DIR} )
add_library ( ${PROJECT_NAME} STATIC ${CMAKE_CURRENT_LIST_DIR}/ccronexpr.c )
target_include_directories ( ${PROJECT_NAME} BEFORE PRIVATE ${${PROJECT_NAME}_INCLUDES} )

# test
enable_testing ( )
add_executable ( ccronexpr_test ${CMAKE_CURRENT_LIST_DIR}/ccronexpr_test.c )
target_include_directories ( ccronexpr_test BEFORE PRIVATE ${${PROJECT_NAME}_INCLUDES} )
target_link_libraries ( ccronexpr_test ${PROJECT_NAME} )
add_test ( ccronexpr_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ccronexpr_test )

# export headers
set ( ${PROJECT_NAME}_INCLUDES ${${PROJECT_NAME}_INCLUDES} CACHE INTERNAL "" )
