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
        ${PROJECT_SOURCE_DIR}/src/presence
        ${PROJECT_SOURCE_DIR}/src/sip
)
target_link_libraries(contactsTest
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Test
)

qt_add_executable(callRoutingTest
    CallRouting.test.cpp
    CallRouting.test.h
    ${PROJECT_SOURCE_DIR}/src/sip/SIPCallRoutingHop.cpp
    ${PROJECT_SOURCE_DIR}/src/sip/SIPCallRoutingHop.h
)

add_test(NAME callRoutingTest COMMAND callRoutingTest)

target_include_directories(callRoutingTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/sip
)
target_link_libraries(callRoutingTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)

qt_add_executable(presenceTest
    Presence.test.cpp
    Presence.test.h
    ${PROJECT_SOURCE_DIR}/src/presence/PresenceStateAggregator.cpp
    ${PROJECT_SOURCE_DIR}/src/presence/PresenceStateAggregator.h
    ${PROJECT_SOURCE_DIR}/src/presence/IPresenceStateProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/presence/IPresenceStateProvider.h
    ${PROJECT_SOURCE_DIR}/src/presence/PresenceState.h
)

add_test(NAME presenceTest COMMAND presenceTest)

target_include_directories(presenceTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/presence
)
target_link_libraries(presenceTest
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Test
)

qt_add_executable(helpersTest
    Helpers.test.cpp
    Helpers.test.h
    ${PROJECT_SOURCE_DIR}/src/helper/TextFormatHelper.cpp
    ${PROJECT_SOURCE_DIR}/src/helper/TextFormatHelper.h
    ${PROJECT_SOURCE_DIR}/src/helper/SecretGenerator.cpp
    ${PROJECT_SOURCE_DIR}/src/helper/SecretGenerator.h
)

add_test(NAME helpersTest COMMAND helpersTest)

target_include_directories(helpersTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/helper
)
target_link_libraries(helpersTest
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Test
)

qt_add_executable(sipTest
    Sip.test.cpp
    Sip.test.h
    ${PROJECT_SOURCE_DIR}/src/sip/PreferredIdentityValidator.cpp
    ${PROJECT_SOURCE_DIR}/src/sip/PreferredIdentityValidator.h
)

add_test(NAME sipTest COMMAND sipTest)

target_include_directories(sipTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/sip
)
target_link_libraries(sipTest
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Test
)

qt_add_executable(usbTest
    Usb.test.cpp
    Usb.test.h
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorParser.cpp
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorParser.h
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorStructs.cpp
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorStructs.h
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorEnums.cpp
    ${PROJECT_SOURCE_DIR}/src/usb/ReportDescriptorEnums.h
)

add_test(NAME usbTest COMMAND usbTest)

target_include_directories(usbTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/usb
)

target_include_directories(usbTest
    SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}/external/hid-rp/hid-rp
)

# Real/synthetic HID report descriptors are loaded from disk using TEST_DATA_DIR at runtime
target_compile_definitions(usbTest
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(usbTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)

qt_add_executable(chatMessageTransformerTest
    ChatMessageTransformer.test.cpp
    ChatMessageTransformer.test.h
    ${PROJECT_SOURCE_DIR}/src/helper/ChatMessageTransformer.cpp
    ${PROJECT_SOURCE_DIR}/src/helper/ChatMessageTransformer.h
)

add_test(NAME chatMessageTransformerTest COMMAND chatMessageTransformerTest)

target_include_directories(chatMessageTransformerTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/helper
)
target_link_libraries(chatMessageTransformerTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)
