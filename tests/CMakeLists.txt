set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(FindPkgConfig)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Network Qml Quick Test Concurrent WebSockets)

if(UNIX)
    pkg_check_modules(LIBAV REQUIRED libavcodec libavutil libswscale IMPORTED_TARGET)
    set(LIBAV_LIBRARIES PkgConfig::LIBAV)
else()
    find_package(ffmpeg REQUIRED)
    set(LIBAV_LIBRARIES ffmpeg::avcodec ffmpeg::avutil ffmpeg::swscale)
endif()

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
        Qt6::Concurrent
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

qt_add_executable(webdavPropfindParserTest
    WebDavPropfinderParser.test.cpp
    WebDavPropfinderParser.test.h
    ${PROJECT_SOURCE_DIR}/src/webdav/WebDavPropfindParser.cpp
    ${PROJECT_SOURCE_DIR}/src/webdav/WebDavPropfindParser.h
    ${PROJECT_SOURCE_DIR}/src/webdav/WebDavItem.cpp
    ${PROJECT_SOURCE_DIR}/src/webdav/WebDavItem.h
)

add_test(NAME webdavPropfindParserTest COMMAND webdavPropfindParserTest)

target_include_directories(webdavPropfindParserTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/webdav
)
target_link_libraries(webdavPropfindParserTest
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

find_package(cmark REQUIRED)

target_include_directories(chatMessageTransformerTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/helper
)
target_link_libraries(chatMessageTransformerTest
    PRIVATE
        Qt6::Core
        Qt6::Test
        cmark::cmark
)

qt_add_executable(cameraFrameSerializerTest
    CameraFrameSerializer.test.cpp
    CameraFrameSerializer.test.h
    ${PROJECT_SOURCE_DIR}/src/ui/CameraFrameSerializer.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/CameraFrameSerializer.h
)

add_test(NAME cameraFrameSerializerTest COMMAND cameraFrameSerializerTest)

target_include_directories(cameraFrameSerializerTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/ui
)
target_link_libraries(cameraFrameSerializerTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Test
)

qt_add_executable(conferenceVideoModelTest
    ConferenceVideoModel.test.cpp
    ConferenceVideoModel.test.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoModel.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoModel.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoOutput.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoOutput.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.h
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.cpp
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.h
)

add_test(NAME conferenceVideoModelTest COMMAND conferenceVideoModelTest)

target_include_directories(conferenceVideoModelTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/conference
        ${PROJECT_SOURCE_DIR}/src/media
)

target_link_libraries(conferenceVideoModelTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Qml
        Qt6::Test
)

qt_add_executable(keyFrameGateTest
    KeyFrameGate.test.cpp
    KeyFrameGate.test.h
    ${PROJECT_SOURCE_DIR}/src/media/EncodedFrame.h
    ${PROJECT_SOURCE_DIR}/src/media/KeyFrameGate.cpp
    ${PROJECT_SOURCE_DIR}/src/media/KeyFrameGate.h
    ${PROJECT_SOURCE_DIR}/src/media/VideoCodec.h
)

add_test(NAME keyFrameGateTest COMMAND keyFrameGateTest)

target_include_directories(keyFrameGateTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/media
)

target_link_libraries(keyFrameGateTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)

qt_add_executable(jitsiSessionProtocolTest
    JitsiSessionProtocol.test.cpp
    JitsiSessionProtocol.test.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceUser.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceUser.h
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiSessionProtocol.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiSessionProtocol.h
)

add_test(NAME jitsiSessionProtocolTest COMMAND jitsiSessionProtocolTest)

target_include_directories(jitsiSessionProtocolTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/conference
)

target_link_libraries(jitsiSessionProtocolTest
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
)

qt_add_executable(jitsiVideoProtocolTest
    JitsiVideoProtocol.test.cpp
    JitsiVideoProtocol.test.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.h
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoProtocol.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoProtocol.h
    ${PROJECT_SOURCE_DIR}/src/media/EncodedFrame.h
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.cpp
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.h
    ${PROJECT_SOURCE_DIR}/src/media/VideoCodec.h
)

add_test(NAME jitsiVideoProtocolTest COMMAND jitsiVideoProtocolTest)

target_include_directories(jitsiVideoProtocolTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/conference
        ${PROJECT_SOURCE_DIR}/src/media
)

target_link_libraries(jitsiVideoProtocolTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Qml
        Qt6::Test
)


qt_add_executable(videoDecoderTest
    IvfReader.h
    VideoDecoder.test.cpp
    VideoDecoder.test.h
    ${PROJECT_SOURCE_DIR}/src/media/LibAvPointers.h
    ${PROJECT_SOURCE_DIR}/src/media/PlanarVideoBuffer.cpp
    ${PROJECT_SOURCE_DIR}/src/media/PlanarVideoBuffer.h
    ${PROJECT_SOURCE_DIR}/src/media/VideoDecoder.cpp
    ${PROJECT_SOURCE_DIR}/src/media/VideoDecoder.h
)

add_test(NAME videoDecoderTest COMMAND videoDecoderTest)

target_include_directories(videoDecoderTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/media
)

# IVF fixtures are loaded from disk using TEST_DATA_DIR at runtime
target_compile_definitions(videoDecoderTest
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(videoDecoderTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Test
        ${LIBAV_LIBRARIES}
)

qt_add_executable(jitsiVideoBridgeTest
    IvfReader.h
    JitsiVideoBridge.test.cpp
    JitsiVideoBridge.test.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoModel.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoModel.h
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/ConferenceVideoSource.h
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoBridge.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoBridge.h
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoProtocol.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoProtocol.h
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoServer.cpp
    ${PROJECT_SOURCE_DIR}/src/conference/JitsiVideoServer.h
    ${PROJECT_SOURCE_DIR}/src/helper/OriginHelper.cpp
    ${PROJECT_SOURCE_DIR}/src/helper/OriginHelper.h
    ${PROJECT_SOURCE_DIR}/src/helper/SecretGenerator.cpp
    ${PROJECT_SOURCE_DIR}/src/helper/SecretGenerator.h
    ${PROJECT_SOURCE_DIR}/src/media/EncodedFrame.h
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.cpp
    ${PROJECT_SOURCE_DIR}/src/media/FramePump.h
    ${PROJECT_SOURCE_DIR}/src/media/KeyFrameGate.cpp
    ${PROJECT_SOURCE_DIR}/src/media/KeyFrameGate.h
    ${PROJECT_SOURCE_DIR}/src/media/LibAvPointers.h
    ${PROJECT_SOURCE_DIR}/src/media/PlanarVideoBuffer.cpp
    ${PROJECT_SOURCE_DIR}/src/media/PlanarVideoBuffer.h
    ${PROJECT_SOURCE_DIR}/src/media/TrackDecoder.cpp
    ${PROJECT_SOURCE_DIR}/src/media/TrackDecoder.h
    ${PROJECT_SOURCE_DIR}/src/media/VideoCodec.h
    ${PROJECT_SOURCE_DIR}/src/media/VideoDecoder.cpp
    ${PROJECT_SOURCE_DIR}/src/media/VideoDecoder.h
)

add_test(NAME jitsiVideoBridgeTest COMMAND jitsiVideoBridgeTest)

target_include_directories(jitsiVideoBridgeTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/conference
        ${PROJECT_SOURCE_DIR}/src/helper
        ${PROJECT_SOURCE_DIR}/src/media
)

# IVF fixtures are loaded from disk using TEST_DATA_DIR at runtime
target_compile_definitions(jitsiVideoBridgeTest
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(jitsiVideoBridgeTest
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Multimedia
        Qt6::Network
        Qt6::Qml
        Qt6::Test
        Qt6::WebSockets
        ${LIBAV_LIBRARIES}
)

if(LINUX)
    pkg_check_modules(LIBSPA REQUIRED libspa-0.2 IMPORTED_TARGET)

    qt_add_executable(pipeWireFrameConverterTest
        PipeWireFrameConverter.test.cpp
        PipeWireFrameConverter.test.h
        ${PROJECT_SOURCE_DIR}/src/platform/linux/PipeWireFrameConverter.cpp
        ${PROJECT_SOURCE_DIR}/src/platform/linux/PipeWireFrameConverter.h
    )

    add_test(NAME pipeWireFrameConverterTest COMMAND pipeWireFrameConverterTest)

    target_include_directories(pipeWireFrameConverterTest
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src/media
            ${PROJECT_SOURCE_DIR}/src/platform/linux
    )
    target_link_libraries(pipeWireFrameConverterTest
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Multimedia
            Qt6::Test
            PkgConfig::LIBSPA
            ${LIBAV_LIBRARIES}
    )
endif()
