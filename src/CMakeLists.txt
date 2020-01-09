project(konq-plugins)

# search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(LibKonq REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_STRICT_ITERATORS)
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

if( NOT LIBKONQ_FOUND )
    MESSAGE(FATAL_ERROR "Install kdebase before to try to compile konq-plugins")
endif( NOT LIBKONQ_FOUND )

add_subdirectory( crashes ) 
add_subdirectory( khtmlsettingsplugin ) 
add_subdirectory( kimgalleryplugin ) 
add_subdirectory( dirfilter ) 
add_subdirectory( uachanger ) 
add_subdirectory( babelfish ) 
add_subdirectory( validators ) 
add_subdirectory( domtreeviewer ) 
macro_ensure_version( "4.1.60" ${KDE_VERSION} KDE4_INSTALLED_VERSION_OK )

if(KDE4_INSTALLED_VERSION_OK)
    add_subdirectory( webarchiver )
else(KDE4_INSTALLED_VERSION_OK)
    MESSAGE(STATUS "webarchiver requires kde trunk")
endif(KDE4_INSTALLED_VERSION_OK)

#add_subdirectory( sidebar ) 
add_subdirectory( imagerotation ) 
add_subdirectory( minitools ) 
#add_subdirectory( microformat ) 
add_subdirectory( autorefresh ) 
add_subdirectory( fsview ) 
add_subdirectory( searchbar ) 
MESSAGE(STATUS "remember to readd sidebar")
add_subdirectory( rellinks ) 

add_subdirectory(adblock)

# Need to port it to dbus and new mediamanager
#add_subdirectory( mediarealfolder ) 

add_subdirectory( akregator ) 


find_package(Msgfmt REQUIRED)
find_package(Gettext REQUIRED)
add_subdirectory( po )
add_subdirectory( doc-translations )
add_subdirectory( doc )
