if(ENABLE_DAV)
    if(UNIX)
        include(FindPkgConfig)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LIBICAL REQUIRED libical IMPORTED_TARGET)

        list(APPEND CALDAV_PLATFORM_LIBRARIES
            PkgConfig::LIBICAL)
    else()
        find_package(libical REQUIRED)

        list(APPEND CALDAV_PLATFORM_LIBRARIES
            libical::libical)
    endif()

    qt_add_plugin(CalDAVEventFeederFactory STATIC
      CLASS_NAME CalDAVEventFeederFactory

      CalDAVEventFeederFactory.cpp
      CalDAVEventFeederFactory.h

      CalDAVEventFeeder.cpp
      CalDAVEventFeeder.h
      CalDAVEventFeederConfig.h
    )

    set_source_files_properties(CalDAVEventFeeder.cpp PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage>")

    target_include_directories(CalDAVEventFeederFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/calendar
        ${PROJECT_SOURCE_DIR}/src/helper
        ${PROJECT_SOURCE_DIR}/src/webdav
    )

    target_link_libraries(CalDAVEventFeederFactory
        PUBLIC
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::Xml
            QCoro6::Core
            QCoro6::Network
            ${CALDAV_PLATFORM_LIBRARIES})

    set(CALDAV_CALENDAR_FACTORY CalDAVEventFeederFactory PARENT_SCOPE)
endif()
