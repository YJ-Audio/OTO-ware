cmake_minimum_required(VERSION 3.22)
project(OTOware VERSION 0.2.0 LANGUAGES C CXX)
set(JUCE_PATH "C:/JUCE" CACHE PATH "JUCE source directory")
add_subdirectory("${JUCE_PATH}" JUCE EXCLUDE_FROM_ALL)
juce_add_plugin(OTOware
    COMPANY_NAME "YJ Audio"
    BUNDLE_ID "com.yjaudio.otoware"
    PLUGIN_MANUFACTURER_CODE Yuit
    PLUGIN_CODE Otwr
    FORMATS VST3 Standalone
    PRODUCT_NAME "OTO-ware"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(OTOware PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(OTOware PUBLIC cxx_std_17)
target_compile_definitions(OTOware PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(OTOware PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(OTOware PUBLIC /utf-8)
endif()
enable_testing()
add_executable(OTOwareTests Tests/ProcessorTests.cpp)
target_include_directories(OTOwareTests PRIVATE Source)
target_link_libraries(OTOwareTests PRIVATE OTOware juce::juce_audio_utils)
add_test(NAME OTOwareProcessor COMMAND OTOwareTests)
