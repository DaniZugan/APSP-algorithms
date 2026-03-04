cmake_minimum_required(VERSION 3.29)
project(APSP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_SYSROOT /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk)

# Use Homebrew's Boost 1.87.0
set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.87.0")  # Homebrew path
find_package(Boost 1.87.0 REQUIRED COMPONENTS system filesystem)  # Match installed version

add_executable(APSP main.cpp
        Dijkstra.cpp
        utils.c
        utils.h
        graphGenerators.c
        graphGenerators.h
        Dijkstra.h
        algoritms/HiddenPaths.cpp
        algoritms/HiddenPaths.h
        algoritms/UniformPaths.cpp
        algoritms/UniformPaths.h
        run_algorithm.c
        run_algorithm.h
        algoritms/Tree.cpp
        algoritms/Tree.h
        algoritms/SCC.cpp
        algoritms/SCC.h
        algoritms/Toroslu.cpp
        algoritms/Toroslu.h
        algoritms/DijkstraAPSP.cpp
        algoritms/DijkstraAPSP.h
        GraphTools/ba_graph.c
        GraphTools/WorstCase.c
        GraphTools/GraphTools.h
        GraphTools/VertexRelax.c
        GraphTools/regular.c
)
target_link_libraries(APSP PRIVATE Boost::boost Boost::system Boost::filesystem)