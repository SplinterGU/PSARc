# Define the C compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)

# Define the C++ compiler
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Define the search paths for MinGW libraries
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Set the operating system to Windows
set(CMAKE_SYSTEM_NAME Windows)

# Set the system type (32 or 64 bits)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the search paths for libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
