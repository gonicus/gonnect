if(NOT EXISTS ${PROJECT_BINARY_DIR}/src/icons.qrc)
    file(GLOB icons RELATIVE "${PROJECT_SOURCE_DIR}" "resources/artwork/icons/*.svg")
    
    set(icons_light "")
    set(icons_dark "")
    
    foreach(icon ${icons})
        cmake_path(GET icon FILENAME _icon)
    
        file(READ ${PROJECT_SOURCE_DIR}/${icon} ICON_DATA)
        string(REPLACE "#232629" "#d3dae3" ICON_DATA "${ICON_DATA}")
        file(WRITE ${PROJECT_BINARY_DIR}/${icon} "${ICON_DATA}")
    
        list(APPEND icons_light "<file alias=\"${_icon}\">${PROJECT_SOURCE_DIR}/${icon}</file>")
        list(APPEND icons_dark "<file alias=\"${_icon}\">${PROJECT_BINARY_DIR}/${icon}</file>")
    endforeach()
    
    string(REPLACE ";" "\n        " ICONS "${icons_light}")
    string(REPLACE ";" "\n        " ICONS_DARK "${icons_dark}")
    
    configure_file(${PROJECT_SOURCE_DIR}/src/icons.qrc.in ${PROJECT_BINARY_DIR}/src/icons.qrc @ONLY)
endif()
