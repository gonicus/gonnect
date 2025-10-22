if(ENABLE_DAV)
    qt_add_plugin(CardDAVAddressBookFactory STATIC
      CLASS_NAME CardDAVAddressBookFactory
    
      CardDAVAddressBookFactory.cpp
      CardDAVAddressBookFactory.h
      CardDAVAddressBookFeeder.cpp
      CardDAVAddressBookFeeder.h
    )
    
    target_include_directories(CardDAVAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )
    
    target_link_libraries(CardDAVAddressBookFactory PUBLIC
      Qt6::Core
      Qt6::Gui
      Qt6::Quick
      Qt6::Widgets
      QtWebDAV
      vCard
      Qt6::Xml)
endif()
