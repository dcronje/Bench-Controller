cmake_minimum_required (VERSION 3.5)

pico_sdk_init()

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c)
file(GLOB_RECURSE HEADER_FILES src/*.h)

if (NOT DEFINED COMPONENT_DIR)

    project (lcdgfx)

    include_directories(src)

    add_library(lcdgfx STATIC ${HEADER_FILES} ${SOURCE_FILES})

    target_link_libraries(lcdgfx 
      pico_stdlib
      hardware_spi
    )
    

else()

    idf_component_register(SRCS ${SOURCE_FILES}
                           INCLUDE_DIRS
                               "src"
                           REQUIRES
                               driver)

endif()
