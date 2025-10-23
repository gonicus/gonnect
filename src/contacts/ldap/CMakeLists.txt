if(ENABLE_LDAP)
    if(UNIX)
        include(FindPkgConfig)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(OPENLDAP REQUIRED IMPORTED_TARGET lber ldap)
        pkg_check_modules(SASL2 IMPORTED_TARGET libsasl2)

        set(LDAP_PLATFORM_LIBS PkgConfig::OPENLDAP)
    else()
        find_package(OpenLDAP REQUIRED)
        set(LDAP_PLATFORM_LIBS OpenLDAP::LDAP)
    endif()

    qt_add_plugin(LDAPAddressBookFactory STATIC
      CLASS_NAME LDAPAddressBookFactory
    
      LDAPAddressBookFactory.cpp
      LDAPAddressBookFactory.h
      LDAPAddressBookFeeder.cpp
      LDAPAddressBookFeeder.h
      LDAPInitializer.cpp
      LDAPInitializer.h
    )

    if(HAVE_SASL)
        target_compile_definitions(LDAPAddressBookFactory PRIVATE HAVE_SASL=1)
    endif()

    target_include_directories(LDAPAddressBookFactory
      PRIVATE
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/dbus/portal
        ${PROJECT_SOURCE_DIR}/src/ui
        ${PROJECT_SOURCE_DIR}/src/sip
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}/src
    )
    
    target_link_libraries(LDAPAddressBookFactory PRIVATE
      PkgConfig::OPENLDAP
      PkgConfig::SASL2
      Qt6::Core
      Qt6::Gui
      Qt6::Quick
      Qt6::Widgets)
    
    set(LDAP_ADDRESSBOOK_FACTORY LDAPAddressBookFactory PARENT_SCOPE)
    set(LDAP_ADDRESSBOOK_LIBRARIES LDAPAddressBookFactory ${OPENLDAP_LIBRARIES} ${SASL2_LIBRARIES} PARENT_SCOPE)
endif()
