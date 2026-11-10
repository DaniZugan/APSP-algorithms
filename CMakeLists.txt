cmake_minimum_required(VERSION 3.20)
project(APSP)

set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    set(CMAKE_OSX_SYSROOT /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk)
    # Use Homebrew's Boost 1.87.0
    set(BOOST_ROOT "/opt/homebrew/Cellar/boost/1.87.0")  # Homebrew path
endif()
find_package(Boost REQUIRED COMPONENTS system filesystem)

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
        GraphTools/worstCase.c
        GraphTools/GraphTools.h
        GraphTools/vertexRelax.c
        GraphTools/regular.c
        GraphTools/randomSCC.c
        GraphTools/snap.c
)
target_link_libraries(APSP PRIVATE Boost::boost Boost::system Boost::filesystem)