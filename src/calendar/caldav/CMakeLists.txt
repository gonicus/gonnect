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

target_link_libraries(CalDAVEventFeederFactory PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::DBus Qt6::Xml)
