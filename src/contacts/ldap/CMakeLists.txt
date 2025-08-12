qt_add_plugin(LDAPAddressBookFactory STATIC
  CLASS_NAME LDAPAddressBookFactory

  LDAPAddressBookFactory.cpp
  LDAPAddressBookFactory.h
  LDAPAddressBookFeeder.cpp
  LDAPAddressBookFeeder.h
)

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
  Qt6::Core
  Qt6::Gui
  Qt6::Quick
  Qt6::Widgets)
