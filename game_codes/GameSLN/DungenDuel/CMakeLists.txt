cmake_minimum_required(VERSION 3.8)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the shared library (DLL)
add_library(GameEngine SHARED "GameEngine.cpp")
target_include_directories(GameEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/json/include
)
target_link_libraries(GameEngine PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(GameEngine PRIVATE GAME_ENGINE_EXPORTS)

# Add the executable
add_executable(DungenDuel "DungenDuel.cpp")
target_link_libraries(DungenDuel PRIVATE GameEngine)

# Copy the DLL to the executable's directory after building
add_custom_command(TARGET DungenDuel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:GameEngine>
        $<TARGET_FILE_DIR:DungenDuel>
    COMMENT "Copying GameEngine.dll to DungenDuel output directory"
)