#if(ENABLE_MSGRAPH)   
    
    qt_add_plugin(MSGraphAddressBookFactory STATIC
      CLASS_NAME MSGraphAddressBookFactory
    
      MSGraphAddressBookFactory.cpp
      MSGraphAddressBookFactory.h
      MSGraphAddressBookFeeder.cpp
      MSGraphAddressBookFeeder.h
    )
    
    target_include_directories(MSGraphAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )
    
    target_link_libraries(MSGraphAddressBookFactory PRIVATE
      Qt6::Core
      Qt6::Gui
      Qt6::Quick      
      Qt6::Xml
      Qt6::NetworkAuth)
#endif()
