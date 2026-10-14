add_library(keramik-decoration MODULE
  keramik.cpp
)

file(GLOB KERAMIK_PICS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} pics/*.png pics/*.svg)
qt_add_resources(keramik-decoration "keramik-pics"
  PREFIX /keramik
  FILES ${KERAMIK_PICS}
)

set_target_properties(keramik-decoration PROPERTIES
  OUTPUT_NAME com.github.neeeeow.klassik.kwin.keramik
)

target_link_libraries(keramik-decoration
  PRIVATE
  Qt6::Svg
  Qt6::Widgets
  KF6::ConfigCore
  KF6::CoreAddons
  KF6::IconThemes
  KDecoration3::KDecoration
)

install(TARGETS keramik-decoration DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})
