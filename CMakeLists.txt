cmake_minimum_required(VERSION 3.22)

project(GuitarPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("C:/Users/jude0/Documents/JUCE" JUCE)

juce_add_plugin(GuitarPlugin
    COMPANY_NAME "Jude"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Jude
    PLUGIN_CODE Gtpl
    FORMATS VST3 Standalone
    PRODUCT_NAME "Guitar Plugin")

juce_generate_juce_header(GuitarPlugin)

target_sources(GuitarPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/NamModel.cpp)

target_compile_definitions(GuitarPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_ASIO=1
        JUCE_VST3_CAN_REPLACE_VST2=0)

set(NAM_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/NeuralAmpModelerCore")

if(NOT EXISTS "${NAM_CORE_DIR}/NAM/get_dsp.h" AND EXISTS "${NAM_CORE_DIR}/NeuralAmpModelerCore-main/NAM/get_dsp.h")
    set(NAM_CORE_DIR "${NAM_CORE_DIR}/NeuralAmpModelerCore-main")
endif()

if(EXISTS "${NAM_CORE_DIR}/NAM/get_dsp.h")
    file(GLOB_RECURSE NAM_CORE_SOURCES CONFIGURE_DEPENDS
        "${NAM_CORE_DIR}/NAM/*.cpp"
        "${NAM_CORE_DIR}/NAM/*.c")

    target_sources(GuitarPlugin PRIVATE ${NAM_CORE_SOURCES})

    target_include_directories(GuitarPlugin
        PRIVATE
            "${NAM_CORE_DIR}"
            "${NAM_CORE_DIR}/NAM"
            "${NAM_CORE_DIR}/Dependencies/eigen"
            "${NAM_CORE_DIR}/Dependencies/nlohmann"
            "${NAM_CORE_DIR}/Dependencies/AudioDSPTools/dsp")

    target_compile_definitions(GuitarPlugin
        PRIVATE
            GUITARPLUGIN_HAS_NAM=1
            NAM_SAMPLE_FLOAT=1
            EIGEN_MAX_ALIGN_BYTES=0
            EIGEN_DONT_VECTORIZE=1)
else()
    message(STATUS "NeuralAmpModelerCore not found. Building with NAM wrapper disabled.")
endif()

target_link_libraries(GuitarPlugin
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

add_custom_command(TARGET GuitarPlugin_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:GuitarPlugin_Standalone>/Assets")

add_custom_command(TARGET GuitarPlugin_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:GuitarPlugin_VST3>/../Resources/Assets")
