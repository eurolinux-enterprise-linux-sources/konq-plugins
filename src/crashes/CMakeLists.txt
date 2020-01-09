

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)


########### next target ###############

set(crashesplugin_PART_SRCS crashesplugin.cpp )

kde4_add_plugin(crashesplugin  ${crashesplugin_PART_SRCS})



target_link_libraries(crashesplugin  ${KDE4_KPARTS_LIBS} )

install(TARGETS crashesplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES crashesplugin.rc crashesplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES crashesplugin.rc crashesplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/webkitpart/kpartplugins ) 

