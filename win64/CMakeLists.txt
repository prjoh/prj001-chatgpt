cmake_minimum_required(VERSION 3.23)

project(chatgpt_game)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${project_SOURCE_DIR}/cmake")
elseif (WIN32)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    set(CMAKE_CXX_FLAGS "-static")
endif()

# Find the SDL2 framework
if (APPLE)
    find_package(SDL2 REQUIRED COMPONENTS SDL2)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(SDL2_mixer REQUIRED)
elseif (WIN32)
    set(SDL2_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2.dll")
    set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")
    set(SDL2_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2.dll")
    find_package(SDL2 REQUIRED)
    set(SDL2_IMAGE_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_image.dll")
    set(SDL2_IMAGE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")
    set(SDL2_IMAGE_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_image.dll")
    find_package(SDL2_image REQUIRED)
    set(SDL2_TTF_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_ttf.dll")
    set(SDL2_TTF_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")
    set(SDL2_TTF_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_ttf.dll")
    find_package(SDL2_ttf REQUIRED)
    set(SDL2_MIXER_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_mixer.dll")
    set(SDL2_MIXER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")
    set(SDL2_MIXER_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2_mixer.dll")
    find_package(SDL2_mixer REQUIRED)
endif()

add_executable(${PROJECT_NAME} main.cpp)

# Add the SDL2 framework to the target
if (APPLE)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)
elseif (WIN32)
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_TTF_LIBRARY} ${SDL2_MIXER_LIBRARY})
endif()

if (APPLE)
    set_target_properties(
            ${PROJECT_NAME} PROPERTIES
            OUTPUT_NAME "game"
            MACOSX_BUNDLE TRUE
    )
    add_custom_command(
            TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res/ game.app/Contents/Resources/res
    )
elseif (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            OUTPUT_NAME ${PROJECT_NAME}
            WIN32 TRUE
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
            )
    # copy the .dll files to the same folder as the executable
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_PATH}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_IMAGE_PATH}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_TTF_PATH}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_MIXER_PATH}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    # copy resources to binary folder
    add_custom_command(
            TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res/
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/
    )
endif()

# Include library headers
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
