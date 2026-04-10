set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(FindPkgConfig)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Test)

add_definitions(-DAPP_TESTS)
enable_testing(true)

qt_add_executable(contactsTest
    Contacts.test.cpp
    Contacts.test.h
    ${PROJECT_SOURCE_DIR}/src/contacts/AddressBook.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/AddressBook.h
    ${PROJECT_SOURCE_DIR}/src/contacts/BlockInfo.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/BlockInfo.h
    ${PROJECT_SOURCE_DIR}/src/contacts/Contact.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/Contact.h
    ${PROJECT_SOURCE_DIR}/src/contacts/PhoneCodeLookup.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/PhoneCodeLookup.h
    ${PROJECT_SOURCE_DIR}/src/contacts/PhoneNumberUtil.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/PhoneNumberUtil.h
    ${PROJECT_SOURCE_DIR}/src/contacts/FuzzyCompare.cpp
    ${PROJECT_SOURCE_DIR}/src/contacts/FuzzyCompare.h
    ${PROJECT_SOURCE_DIR}/src/ReadOnlyConfdSettings.h
    ${PROJECT_SOURCE_DIR}/src/ReadOnlyConfdSettings.cpp
)

add_test(NAME contactsTest COMMAND contactsTest)

target_include_directories(contactsTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/contacts
        ${PROJECT_SOURCE_DIR}/src/sip
)
target_link_libraries(contactsTest
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Test
)
