find_package(ECM NO_MODULE)
if(ECM_MODULE_DIR)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})

    find_package(KPim6Akonadi CONFIG)
    find_package(KPim6PimCommonAkonadi CONFIG)
    find_package(KF6CalendarCore CONFIG)
    find_package(KPim6AkonadiCalendar CONFIG)
    find_package(KPim6AkonadiSearch CONFIG)

    if(KF6CalendarCore_FOUND AND KPim6AkonadiCalendar_FOUND AND KPim6AkonadiSearch_FOUND)
        set(AKONADI_CALENDAR_LIBS KPim6::AkonadiCalendar KF6::CalendarCore)

        qt_add_plugin(AkonadiEventFeederFactory STATIC
          CLASS_NAME AkonadiEventFeederFactory

          AkonadiEventFeederFactory.cpp
          AkonadiEventFeederFactory.h
          AkonadiEventFeeder.cpp
          AkonadiEventFeeder.h
        )
        
        target_include_directories(AkonadiEventFeederFactory
          PRIVATE
            ${PROJECT_SOURCE_DIR}/src/calendar
            ${PROJECT_SOURCE_DIR}/src/ui
            ${PROJECT_SOURCE_DIR}/src/sip
            ${PROJECT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}
            ${PROJECT_BINARY_DIR}/src
        )
        
        target_link_libraries(AkonadiEventFeederFactory PUBLIC Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::DBus ${AKONADI_CALENDAR_LIBS})

        set(AKONADI_CALENDAR_FACTORY AkonadiEventFeederFactory PARENT_SCOPE)
    endif()
endif()
