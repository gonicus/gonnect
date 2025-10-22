if(ENABLE_EDS)
    pkg_check_modules(GLIB REQUIRED glib-2.0)
    pkg_check_modules(EDS REQUIRED libecal-2.0 libedata-cal-2.0)

    qt_add_plugin(EDSEventFeederFactory STATIC
      CLASS_NAME EDSEventFeederFactory
    
      EDSEventFeederFactory.cpp
      EDSEventFeederFactory.h
      EDSEventFeeder.cpp
      EDSEventFeeder.h
    )
    
    target_include_directories(EDSEventFeederFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/calendar
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
        ${GLIB_INCLUDE_DIRS}
        ${EDS_INCLUDE_DIRS}
    )
    
    target_link_libraries(EDSEventFeederFactory
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::Widgets
            Qt6::DBus
            ${GLIB_LIBRARIES}
            ${EDS_LIBRARIES})

    set(EDS_CALENDAR_FACTORY EDSEventFeederFactory PARENT_SCOPE)
    set(EDS_CALENDAR_LIBRARIES EDSEventFeederFactory ${GLIB_LIBRARIES} ${EDS_LIBRARIES} PARENT_SCOPE)
endif()
