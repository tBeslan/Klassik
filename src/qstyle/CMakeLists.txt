# Keramik tiles are converted at build time into recolorable data tables
add_executable(keramik-genembed keramik/genembed.cpp)
target_link_libraries(keramik-genembed PRIVATE Qt6::Gui)

file(GLOB KERAMIK_PICS ${CMAKE_CURRENT_SOURCE_DIR}/keramik/pics/*.png)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pixmaps.keramik ${CMAKE_CURRENT_BINARY_DIR}/keramikrc.h
  COMMAND keramik-genembed ${CMAKE_CURRENT_BINARY_DIR}/pixmaps.keramik ${CMAKE_CURRENT_BINARY_DIR}/keramikrc.h ${KERAMIK_PICS}
  DEPENDS keramik-genembed ${KERAMIK_PICS}
)

add_library(klassik-style MODULE
  klassikstyle.cpp
  klassikstyleplugin.cpp
  keramik/keramikstyle.cpp
  keramik/pixmaploader.cpp
  keramik/gradients.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/pixmaps.keramik
  ${CMAKE_CURRENT_BINARY_DIR}/keramikrc.h
)

set_target_properties(klassik-style PROPERTIES
  OUTPUT_NAME com.github.neeeeow.klassik.qstyle
)

target_include_directories(klassik-style PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/keramik)

target_link_libraries(klassik-style PRIVATE
  Qt6::Widgets
)

install(TARGETS klassik-style DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/styles/)
