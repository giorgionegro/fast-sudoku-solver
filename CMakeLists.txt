cmake_minimum_required(VERSION 3.17)
project(sudoku)

set(CMAKE_CXX_STANDARD 20)

# set optimization level to 3 and march=native unroll loops
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -march=native -fopenmp -pthread -O3  -funroll-loops ")
#add_executable(sudoku main.cpp)


# create a profile generate executable and run it to generate a profile then run the profile
add_executable(sudoku main.cpp)




# Target for profile generation
add_custom_target(pgo_generate
        COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="${CMAKE_CXX_FLAGS} -fprofile-generate" LDFLAGS=-fprofile-generate ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        COMMAND ./sudoku
        COMMENT "Generating profile data"
)

# Target for using the generated profile
add_custom_target(pgo_use
        COMMAND ${CMAKE_COMMAND} -E env CXXFLAGS="${CMAKE_CXX_FLAGS} -fprofile-use " LDFLAGS=-fprofile-use  ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        COMMENT "Building with profile-guided optimizations"
)

# Combined target to first generate the profile and then use it
add_custom_target(pgo
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target pgo_generate
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target pgo_use
        COMMENT "Running PGO (generate + use)"
        DEPENDS pgo_generate pgo_use
)