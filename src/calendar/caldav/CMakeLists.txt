find_package(QtWebDAV REQUIRED)

qt_add_plugin(CalDAVEventFeederFactory STATIC
  CLASS_NAME CalDAVEventFeederFactory

  CalDAVEventFeeder.cpp
  CalDAVEventFeeder.h
  CalDAVEventFeederConfig.h

  CalDAVEventFeederFactory.cpp
  CalDAVEventFeederFactory.h
)

target_include_directories(CalDAVEventFeederFactory
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/calendar
)

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

target_link_libraries(CalDAVEventFeederFactory PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Quick
  Qt6::Widgets
  Qt6::Xml
  QtWebDAV
  ${CALDAV_PLATFORM_LIBRARIES})
