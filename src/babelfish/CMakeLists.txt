


include(MacroOptionalDependPackage)

MACRO_OPTIONAL_DEPEND_PACKAGE(KdeWebKit "kdewebkit")
if(DEPEND_PACKAGE_KdeWebKit)
        macro_optional_find_package(KdeWebKit QUIET)
else(DEPEND_PACKAGE_KdeWebKit)
        set(KDEWEBKIT_FOUND FALSE)
endif(DEPEND_PACKAGE_KdeWebKit)

MACRO_OPTIONAL_DEPEND_PACKAGE(WebKitPart "webkitpart")
if(DEPEND_PACKAGE_WebKitPart)
        macro_optional_find_package(WebKitPart QUIET)
else(DEPEND_PACKAGE_WebKitPart)
        set(WEBKITPART_FOUND FALSE)
endif(DEPEND_PACKAGE_WebKitPart)

if( WEBKITPART_FOUND AND KDEWEBKIT_FOUND)
    set(WEBKITKDE_FOUND true)
endif( WEBKITPART_FOUND AND KDEWEBKIT_FOUND)


macro_log_feature(WEBKITKDE_FOUND "WebKitKde" "Webkitkde libraries and header files" "" FALSE "" "Webkitkde is necessary to compile support for WebKitKde.")

macro_bool_to_01(WEBKITKDE_FOUND HAVE_WEBKITKDE)


if( WEBKITKDE_FOUND )
    include_directories(${KDEWEBKIT_INCLUDE_DIR})
    include_directories(${WEBKITPART_INCLUDE_DIR})
endif(WEBKITKDE_FOUND)




configure_file(config-babelfish.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-babelfish.h )

########### next target ###############

set(babelfishplugin_PART_SRCS plugin_babelfish.cpp )

kde4_add_plugin(babelfishplugin ${babelfishplugin_PART_SRCS})



target_link_libraries(babelfishplugin  ${KDE4_KHTML_LIBS} )
if( WEBKITKDE_FOUND)
    target_link_libraries(babelfishplugin ${WEBKITPART_LIBRARIES} ${KDEWEBKIT_LIBRARIES} ${QT_QTWEBKIT_LIBRARY} )
endif(WEBKITKDE_FOUND)


install(TARGETS babelfishplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_babelfish.rc plugin_babelfish.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES translaterc  DESTINATION  ${CONFIG_INSTALL_DIR} )
if(WEBKITKDE_FOUND)
   install( FILES plugin_babelfish.rc plugin_babelfish.desktop  DESTINATION  ${DATA_INSTALL_DIR}/webkitpart/kpartplugins )
endif(WEBKITKDE_FOUND)
kde4_install_icons( ${ICON_INSTALL_DIR}   )

