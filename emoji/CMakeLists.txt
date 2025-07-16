find_package(Qt6 REQUIRED)

add_executable(update-emojis
    main.cpp
    EmojiLoader.h
    EmojiLoader.cpp
    ../src/ui/EmojiInfo.h
    ../src/ui/EmojiInfo.cpp
)

target_link_libraries(update-emojis
    Qt${QT_VERSION_MAJOR}::Core
)

include(GNUInstallDirs)

install(TARGETS update-emojis
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

find_program(CURL_EXECUTABLE curl)
if(CURL_EXECUTABLE)
    add_custom_target(download-emojis
      COMMAND ${CURL_EXECUTABLE} -sL -o ${CMAKE_CURRENT_SOURCE_DIR}/resources/data.json https://cdn.jsdelivr.net/npm/emojibase-data@16.0.3/en/data.json
      COMMAND ${CURL_EXECUTABLE} -sL -o ${CMAKE_CURRENT_SOURCE_DIR}/resources/emojibase.json https://cdn.jsdelivr.net/npm/emojibase-data@16.0.3/en/shortcodes/emojibase.json)

    add_custom_target(gen-emojis-dat
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/update-emojis ${CMAKE_CURRENT_SOURCE_DIR}/resources/ ${PROJECT_SOURCE_DIR}/resources/emojis/
      DEPENDS update-emojis download-emojis
      VERBATIM)
endif()
