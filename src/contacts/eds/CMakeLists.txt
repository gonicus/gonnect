# EDS plugin
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(EDS libebook-1.2 libedata-book-1.2)

if("${GLIB_LIBRARIES}" STREQUAL "" OR "${EDS_LIBRARIES}" STREQUAL "")
    message(STATUS "Skipping EDS build - no libraries found")
else()
    qt_add_plugin(EDSAddressBookFactory STATIC
      CLASS_NAME EDSAddressBookFactory
    
      EDSAddressBookFactory.cpp
      EDSAddressBookFactory.h
      EDSAddressBookFeeder.cpp
      EDSAddressBookFeeder.h
    )
    
    target_include_directories(EDSAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
        ${GLIB_INCLUDE_DIRS}
        ${EDS_INCLUDE_DIRS}
    )
    
    target_link_libraries(EDSAddressBookFactory
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::Widgets
            Qt6::DBus
            ${GLIB_LIBRARIES}
            ${EDS_LIBRARIES})

    set(EDS_ADDRESSBOOK_FACTORY EDSAddressBookFactory PARENT_SCOPE)
    set(EDS_ADDRESSBOOK_LIBRARIES EDSAddressBookFactory ${GLIB_LIBRARIES} ${EDS_LIBRARIES} PARENT_SCOPE)
endif()
