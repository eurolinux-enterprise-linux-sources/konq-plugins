
include_directories( ${KDE4_INCLUDE_DIR}/dom  )


########### next target ###############

set(konqsidebar_metabar_PART_SRCS 
   metabar.cpp 
   metabarwidget.cpp 
   configdialog.cpp 
   serviceloader.cpp 
   defaultplugin.cpp 
   protocolplugin.cpp 
   settingsplugin.cpp 
   remoteplugin.cpp 
   httpplugin.cpp 
   metabarfunctions.cpp )

kde4_add_plugin(konqsidebar_metabar ${konqsidebar_metabar_PART_SRCS})



target_link_libraries(konqsidebar_metabar konqsidebarplugin ${KDE4_KHTML_LIBS}
    ${KDE4_KPARTS_LIBS} )

install(TARGETS konqsidebar_metabar  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES metabar.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries )
install( FILES default.css layout.html  DESTINATION  ${DATA_INSTALL_DIR}/metabar/themes/default )
install( FILES iconsrc  DESTINATION  ${DATA_INSTALL_DIR}/metabar )
install( FILES metabar_add.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/add )

kde4_install_icons( ${ICON_INSTALL_DIR}   )



