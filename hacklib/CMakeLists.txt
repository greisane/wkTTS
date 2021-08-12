project(hacklib)

include_directories(hacklib PUBLIC include)

add_library(hacklib STATIC
    src/ExeFile.cpp
    src/ExeFile_WIN32.cpp
    src/PatternScanner.cpp
    src/Memory_WIN32.cpp
)
