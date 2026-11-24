cmake_minimum_required(VERSION 3.16)
project(future-task-manager VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts LinguistTools)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES pt_BR es)

qt_add_executable(future-task-manager src/main.cpp)
target_link_libraries(future-task-manager PRIVATE Qt6::Widgets Qt6::Charts)

qt_add_translations(future-task-manager
    TS_FILES i18n/future-task-manager_pt_BR.ts i18n/future-task-manager_es.ts
    RESOURCE_PREFIX "/i18n"
)

include(GNUInstallDirs)
install(TARGETS future-task-manager DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/future-task-manager.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES data/future-task-manager.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)

set(CPACK_PACKAGE_NAME "future-task-manager")
set(CPACK_PACKAGE_VENDOR "Future Technology Informática")
set(CPACK_PACKAGE_CONTACT "Future Technology Informática <contato@futuretechnology.com.br>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-platform System Performance Monitor")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x86_64")

set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_AUTOREQ "YES")

include(CPack)
