cmake_minimum_required(VERSION 3.17)
project(sudoku)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -march=native -mtune=native -flto  -Ofast -fopt-info-all  -ftree-vectorizer-verbose=2 -funroll-loops  -fprofile-use")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")


-add_executable(sudoku main.cpp)




add_custom_target(pgo_generate
        COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="${CMAKE_CXX_FLAGS} -fprofile-generate" LDFLAGS=-fprofile-generate ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        COMMAND ./sudoku /mnt/c/Users/giorgio/CLionProjects/sudoku/sudoku_input.txt
        COMMENT "Generating profile data"
)
add_custom_target(pgo_use
        COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="${CMAKE_CXX_FLAGS} -fprofile-use " LDFLAGS=-fprofile-use  ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        COMMENT "Building with profile-guided optimizations"
)

add_custom_target(pgo
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target pgo_generate
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target pgo_use
        COMMENT "Running PGO (generate + use)"
        DEPENDS pgo_generate pgo_use
)