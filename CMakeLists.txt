cmake_minimum_required(VERSION 3.19)
project(wkTTS
    VERSION "1.0.0.0"
    DESCRIPTION "Text-to-speech for game chat"
)

add_subdirectory(flite)
add_subdirectory(hacklib)

set(CMAKE_CXX_STANDARD 20)
configure_file(resource.rc.in resource.rc)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")
add_compile_definitions(PROJECT_VERSION="${PROJECT_VERSION}")

include_directories(include hacklib/include flite/inc)
link_directories(lib)

add_library(wkTTS SHARED
    src/dllmain.cpp
    src/Hooks.cpp
    src/Speaker.cpp
    src/LobbyChat.cpp
    src/GameChat.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/resource.rc
)

target_link_libraries(wkTTS
    PUBLIC hacklib
    PUBLIC libMinHook-x86-v141-mt
    PUBLIC flite_audio
    PUBLIC flite_hrg
    PUBLIC flite_regex
    PUBLIC flite_stats
    PUBLIC flite_utils
    PUBLIC flite_cg
    PUBLIC flite_lexicon
    PUBLIC flite_speech
    PUBLIC flite_synth
    PUBLIC flite_wavesynth
    PUBLIC flite_cmu_grapheme_lang
    PUBLIC flite_cmu_grapheme_lex
    PUBLIC flite_cmulex
    PUBLIC flite_cmu_us_kal
    PUBLIC flite_cmu_us_kal16
    PUBLIC flite_cmu_us_slt
    PUBLIC flite_cmu_us_rms
    PUBLIC flite_cmu_us_awb
    PUBLIC flite_usenglish
)

# set(GAME_DIR "D:/WA")
# add_custom_command(TARGET wkTTS
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wkTTS> ${GAME_DIR}/$<TARGET_FILE_NAME:wkTTS>
#     COMMAND "${GAME_DIR}/WA.exe" "wa://?id=tts?pass=tts"
# )
