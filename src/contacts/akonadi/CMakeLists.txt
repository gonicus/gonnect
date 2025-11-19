if(ENABLE_AKONADI)
    find_package(ECM REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_DIR})
    
    find_package(KPim6Akonadi REQUIRED CONFIG)
    find_package(KPim6PimCommonAkonadi REQUIRED CONFIG)
    find_package(KPim6AkonadiSearch REQUIRED CONFIG)
    
    set(AKONADI_CONTACT_LIBS KPim6::AkonadiContactCore)
    
    qt_add_plugin(AkonadiAddressBookFactory STATIC
      CLASS_NAME AkonadiAddressBookFactory
    
      AkonadiAddressBookFactory.cpp
      AkonadiAddressBookFactory.h
      AkonadiAddressBookFeeder.cpp
      AkonadiAddressBookFeeder.h
    )
    
    target_include_directories(AkonadiAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )
    
    target_link_libraries(AkonadiAddressBookFactory PUBLIC Qt6::Core Qt6::Gui Qt6::Quick Qt6::DBus ${AKONADI_CONTACT_LIBS})
    
    set(AKONADI_CONTACT_FACTORY AkonadiAddressBookFactory PARENT_SCOPE)
endif()
