cmake_minimum_required(VERSION 3.0)
project(psar)

# Add your source files
set(SOURCES
    src/main.c
    src/pak.c
    src/unpak.c
    src/md5.c
    src/inettypes.c
    src/common.c
    src/file_utils.c
    src/hashset.c
    src/report.c
    src/threads.c
)

# Set the executable output
add_executable(psar ${SOURCES})

# Check the operating system to add the appropriate libraries
if(WIN32)
    target_link_libraries(psar PRIVATE -lz -lws2_32 -lpthread --static -llzma)
elseif(UNIX)
    target_link_libraries(psar PRIVATE -lz -llzma)
endif()

# Enable "strip" for the executable
if(CMAKE_COMPILER_IS_GNUCXX)
    add_custom_command(TARGET psar POST_BUILD
        COMMAND strip $<TARGET_FILE:psar>
    )
endif()
