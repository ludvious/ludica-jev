cmake_minimum_required(VERSION 3.16)
project(ludica-jev LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib is fetched and built locally so the project needs no system install.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

# Decision-model client dependencies (both header-only, no system install).
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.20.0
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cpp-httplib)

add_executable(ludica-jev
  src/main.cpp
  src/board.cpp
  src/expectimax.cpp
  src/decision_model.cpp
)
target_link_libraries(ludica-jev PRIVATE raylib httplib::httplib nlohmann_json::nlohmann_json)

# Board logic is pure stdlib; the test target does not need raylib.
enable_testing()
add_executable(test_board tests/test_board.cpp src/board.cpp)
add_test(NAME board COMMAND test_board)

add_executable(test_expectimax tests/test_expectimax.cpp src/board.cpp src/expectimax.cpp)
add_test(NAME expectimax COMMAND test_expectimax)

add_executable(test_decision_model tests/test_decision_model.cpp src/board.cpp src/decision_model.cpp)
target_link_libraries(test_decision_model PRIVATE httplib::httplib nlohmann_json::nlohmann_json)
add_test(NAME decision_model COMMAND test_decision_model)

# Headless benchmark: plays N games per policy and prints score stats.
add_executable(bench bench/bench.cpp src/board.cpp src/expectimax.cpp src/decision_model.cpp)
target_link_libraries(bench PRIVATE httplib::httplib nlohmann_json::nlohmann_json)
