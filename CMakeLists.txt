cmake_minimum_required(VERSION 3.24)

project(FTRemote VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(FTREMOTE_BUILD_TESTS "Build FT Remote tests" ON)
option(FTREMOTE_USE_QTKEYCHAIN "Use QtKeychain when it is installed" ON)
option(FTREMOTE_FETCH_QTKEYCHAIN "Fetch and build QtKeychain when no package is installed" OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network WebSockets Multimedia LinguistTools)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

add_library(ftremote_core
    src/core/protocol_types.cpp
    src/core/protocol_types.h
    src/core/media_parser.cpp
    src/core/media_parser.h
    src/core/cat_catalog.cpp
    src/core/cat_catalog.h
    src/core/credential_store.cpp
    src/core/credential_store.h
    src/core/session_client.cpp
    src/core/session_client.h
    src/core/audio_engine.cpp
    src/core/audio_engine.h
)

target_include_directories(ftremote_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ftremote_core
    PUBLIC
        Qt6::Core Qt6::Network Qt6::WebSockets Qt6::Multimedia
)

if(FTREMOTE_USE_QTKEYCHAIN)
    find_package(Qt6Keychain QUIET)
    if(NOT Qt6Keychain_FOUND AND FTREMOTE_FETCH_QTKEYCHAIN)
        include(FetchContent)
        FetchContent_Declare(qtkeychain
            GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
            GIT_TAG 0.17.0
        )
        set(BUILD_WITH_QT5 OFF CACHE BOOL "" FORCE)
        set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
        set(BUILD_TEST_APPLICATION OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(qtkeychain)
        if(TARGET qt6keychain)
            if(WIN32)
                set_target_properties(qt6keychain PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
                )
            endif()
            set(Qt6Keychain_FOUND TRUE)
            set(FTREMOTE_QTKEYCHAIN_TARGET qt6keychain)
        endif()
    endif()
    if(Qt6Keychain_FOUND)
        if(NOT FTREMOTE_QTKEYCHAIN_TARGET)
            set(FTREMOTE_QTKEYCHAIN_TARGET Qt6Keychain::Qt6Keychain)
        endif()
        target_link_libraries(ftremote_core PRIVATE ${FTREMOTE_QTKEYCHAIN_TARGET})
        target_compile_definitions(ftremote_core PRIVATE FTREMOTE_HAS_QTKEYCHAIN=1)
    else()
        message(STATUS "QtKeychain not found; password persistence is disabled at runtime")
    endif()
endif()

qt_add_executable(ftremote
    src/main.cpp
    src/app_controller.cpp
    src/app_controller.h
)

qt_add_qml_module(ftremote
    URI FTRemote
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Login.qml
        qml/ControlsPanel.qml
        qml/SpectrumView.qml
        qml/WaterfallView.qml
)

qt_add_translations(ftremote
    TS_FILES
        translations/ftremote_zh_CN.ts
        translations/ftremote_en.ts
)

target_link_libraries(ftremote PRIVATE ftremote_core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

set_target_properties(ftremote PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS ftremote
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# QtKeychain is a non-Qt shared library. Qt's generated deployment script
# deploys Qt frameworks and plugins, but it does not reliably install a
# FetchContent-built third-party dylib into the application bundle.
if(APPLE AND TARGET qt6keychain)
    install(TARGETS qt6keychain
        LIBRARY DESTINATION ftremote.app/Contents/Frameworks
        RUNTIME DESTINATION ftremote.app/Contents/Frameworks
    )
endif()

if(COMMAND qt_generate_deploy_qml_app_script)
    qt_generate_deploy_qml_app_script(
        TARGET ftremote
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

if(FTREMOTE_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(ftremote_protocol_tests tests/test_protocol.cpp)
    target_link_libraries(ftremote_protocol_tests PRIVATE ftremote_core Qt6::Test)
    add_test(NAME ftremote_protocol_tests COMMAND ftremote_protocol_tests)
    add_executable(ftremote_media_tests tests/test_media_parser.cpp)
    target_link_libraries(ftremote_media_tests PRIVATE ftremote_core Qt6::Test)
    add_test(NAME ftremote_media_tests COMMAND ftremote_media_tests)
    if(UNIX AND NOT APPLE)
        set_tests_properties(ftremote_protocol_tests ftremote_media_tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )
    endif()
endif()

include(CPack)
set(CPACK_PACKAGE_NAME "FT Remote")
set(CPACK_PACKAGE_VENDOR "FT Remote Project")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ")
endif()
