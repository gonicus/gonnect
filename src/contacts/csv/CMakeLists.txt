if(ENABLE_CSV)
    qt_add_plugin(CSVAddressBookFactory STATIC
      CLASS_NAME CSVAddressBookFactory
    
      CSVAddressBookFactory.cpp
      CSVAddressBookFactory.h
      CSVFileAddressBookFeeder.cpp
      CSVFileAddressBookFeeder.h
    )
    
    target_include_directories(CSVAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )
    
    target_link_libraries(CSVAddressBookFactory PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets)
endif()
