if(ENABLE_MSGRAPH)
    qt_add_plugin(MSGraphEventFeederFactory STATIC
      CLASS_NAME MSGraphEventFeederFactory

      MSGraphEventFeederFactory.cpp
      MSGraphEventFeederFactory.h
      MSGraphEventFeeder.cpp
      MSGraphEventFeeder.h
    )

    target_include_directories(MSGraphEventFeederFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/calendar
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )

    target_link_libraries(MSGraphEventFeederFactory PRIVATE
      Qt6::Core
      Qt6::Gui
      Qt6::Quick
      Qt6::Xml
      Qt6::NetworkAuth)

    set(MSGRAPH_CALENDAR_FACTORY MSGraphEventFeederFactory PARENT_SCOPE)
endif()
