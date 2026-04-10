set(CPACK_PACKAGE_VENDOR "${APPLICATION_VENDOR}")
set(CPACK_PACKAGE_NAME "${APPLICATION_NAME}")
set(CPACK_PACKAGE_DESCRIPTION "GOnnect a modern, easy to use UC client.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GOnnect a modern, easy to use UC client.")
set(CPACK_PACKAGE_EXECUTABLES "${APPLICATION_EXECUTABLE}" "${APPLICATION_NAME}")
set(CPACK_PROJECT_NAME gonnect)
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_STRIP_FILES TRUE)

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${APPLICATION_NAME}")
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/resources/artwork/gonnect.ico")
    set(CPACK_NSIS_DISPLAY_NAME "${APPLICATION_NAME}")
    set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/resources/windows/InstallerIcon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/resources/windows/InstallerIcon.ico")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${APPLICATION_EXECUTABLE}.exe")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CPACK_NSIS_EXECUTABLES_DIRECTORY}\\${CPACK_NSIS_MUI_FINISHPAGE_RUN}")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${APPLICATION_NAME}")
    set(CPACK_NSIS_URL_INFO_ABOUT ${APPLICATION_URL})

    if("$ENV{NSIS_USE_FAST_COMPRESSION}" STREQUAL "1")
        set(CPACK_NSIS_COMPRESSOR ZLIB)
    endif()

    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" DOS_STYLE_BINARY_DIR)
    configure_file("${CMAKE_SOURCE_DIR}/resources/windows/install.nsi.in" "${CMAKE_CURRENT_BINARY_DIR}/install.nsi" @ONLY)
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "!include \"${DOS_STYLE_BINARY_DIR}\\install.nsi\"")
    configure_file("${CMAKE_SOURCE_DIR}/resources/windows/uninstall.nsi.in" "${CMAKE_CURRENT_BINARY_DIR}/uninstall.nsi" @ONLY)

    configure_file("${CMAKE_SOURCE_DIR}/resources/windows/strings.nsi.in" "${CMAKE_CURRENT_BINARY_DIR}/strings.nsi" @ONLY)
    
    # not really the correct variable, but it is the right place (and unused)
    set(CPACK_NSIS_INSTALLATION_TYPES "!include \"${DOS_STYLE_BINARY_DIR}\\strings.nsi\"")
    
    set(CPACK_NSIS_DEFINES "\
  !include \"x64.nsh\"
  !include \"FileFunc.nsh\"

  !define MUI_FINISHPAGE_SHOWREADME
  !define MUI_FINISHPAGE_SHOWREADME_TEXT \"$(ADD_AUTOSTART)\"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION \"Add_Autostart\"
  
  Var processFound

  !macro FindProc
      ExecDos::exec /TOSTACK 'cmd /C tasklist /NH /FI \"IMAGENAME eq ${APPLICATION_EXECUTABLE}.exe\" | find /I \"${APPLICATION_EXECUTABLE}.exe\"\'
      Pop $processFound ; The exit code      
  !macroend

  Function Add_Autostart
    SetOutPath \"$INSTDIR\"    
    WriteRegStr HKCU \"Software\\Microsoft\\Windows\\CurrentVersion\\Run\" \"${APPLICATION_EXECUTABLE}\" \"$INSTDIR\\GOnnect.lnk\"
  FunctionEnd
  
    ")

    configure_file("${CMAKE_SOURCE_DIR}/resources/windows/pre-install.nsi.in" "${CMAKE_CURRENT_BINARY_DIR}/pre-install.nsi" @ONLY)
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "\
  \$\{If\} \$\{RunningX64\}
    SetRegView 64
  \$\{EndIf\}
  !include \"${DOS_STYLE_BINARY_DIR}\\pre-install.nsi\"
    ")

    list(APPEND CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\
  \$\{If\} \$\{RunningX64\}
    SetRegView 64
  \$\{EndIf\}
  !include \"${DOS_STYLE_BINARY_DIR}\\uninstall.nsi\"
    ")

    install(FILES "${CMAKE_SOURCE_DIR}/resources/windows/EventLog.dll" DESTINATION "${CPACK_NSIS_EXECUTABLES_DIRECTORY}")

elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/resources/macos/VolumeIcon.icns")
    set(CPACK_BUNDLE_ICON ${CMAKE_PACKAGE_ICON})
    set(CPACK_DMG_BACKGROUND_IMAGE "${PROJECT_SOURCE_DIR}/resources/macos/background.png")
    set(CPACK_DMG_VOLUME_NAME "${APPLICATION_NAME} Install")
    #set(CPACK_DMG_FORMAT "UDRW") # Enable to edit .DS_Store
    set(CPACK_DMG_DS_STORE "${PROJECT_SOURCE_DIR}/resources/macos/DS_Store")
endif()

include(CPack)
