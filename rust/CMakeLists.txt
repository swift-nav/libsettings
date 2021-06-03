cmake_minimum_required(VERSION 3.2)

add_library(
   rustbindsettings
   STATIC
   libsettings_locks.c
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(rustbindsettings Threads::Threads)

target_include_directories(rustbindsettings
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/..
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../third_party/libsbp/c/include
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../include
)

install(TARGETS rustbindsettings
  ARCHIVE DESTINATION lib
  )
