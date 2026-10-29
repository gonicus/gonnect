if(ENABLE_DAV)
    find_package(vCard REQUIRED)

    qt_add_plugin(CardDAVAddressBookFactory STATIC
      CLASS_NAME CardDAVAddressBookFactory

      CardDAVAddressBookFactory.cpp
      CardDAVAddressBookFactory.h

      CardDAVAddressBookFeeder.cpp
      CardDAVAddressBookFeeder.h
      CardDAVAddressBookFeederConfig.h
    )

    set_source_files_properties(CardDAVAddressBookFeeder.cpp PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage>")

    target_include_directories(CardDAVAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/presence
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src/helper
        ${PROJECT_SOURCE_DIR}/src/webdav
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )

    target_link_libraries(CardDAVAddressBookFactory PRIVATE
      Qt6::Core
      Qt6::Gui
      Qt6::Quick
      Qt6::Xml
      QCoro6::Core
      QCoro6::Network
      vCard)

    set(CARDDAV_CONTACT_FACTORY CardDAVAddressBookFactory PARENT_SCOPE)
endif()
