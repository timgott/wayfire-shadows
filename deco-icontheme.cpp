#include "deco-icontheme.hpp"
#include "INIReader.h"

#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <wayfire/config.h>

#include <filesystem>
#include <sstream>

bool exists( std::string path ) {

    struct stat statbuf;
	if ( stat( path.c_str(), &statbuf ) == 0 )

		if ( S_ISDIR( statbuf.st_mode ) )
			return ( access( path.c_str(), R_OK | X_OK ) == 0 );

		else if ( S_ISREG( statbuf.st_mode ) )
			return ( access( path.c_str(), R_OK ) == 0 );

        else
            return false;

	else
		return false;
}

std::vector<std::string> getDesktops( std::string path ) {

    std::vector<std::string> desktops;
    for (const auto & entry : std::filesystem::directory_iterator( path ) ) {
        if ( entry.is_regular_file() and entry.path().extension() == ".desktop" )
            desktops.push_back( entry.path() );
    }

    return desktops;
};

wf::windecor::IconThemeManager::IconThemeManager( std::string iconTheme ) {

    setIconTheme( iconTheme );
};

void wf::windecor::IconThemeManager::setIconTheme( std::string iconThemeName ) {

    if ( mIconTheme == iconThemeName )
        return;

    std::string home_local_share = getenv( "HOME" );
    home_local_share += "/.local/share/icons/";

    std::vector<std::string> icoDirs = {
        home_local_share,
        "/usr/local/share/icons/",
        "/usr/share/icons/",
    };

    /* We use hicolor as the theme if @iconThemeName is empty */
    if ( iconThemeName.empty() )
        mIconTheme = "/usr/share/icons/hicolor/";

    else {
        for( std::string dir: icoDirs ) {
            if ( exists( dir + iconThemeName ) ) {
                mIconTheme = dir + iconThemeName + "/";
                break;
            }
        }

        if ( mIconTheme.empty() ) {
            if ( exists( "/usr/share/icons/Adwaita" ) )
                mIconTheme = "/usr/share/icons/Adwaita/";

            else if ( exists( "/usr/share/icons/breeze" ) )
                mIconTheme = "/usr/share/icons/breeze/";

            else
                mIconTheme = "/usr/share/icons/hicolor/";
        }
    }

    /* Fallback themes */
    std::vector<std::string> themes = { mIconTheme };
    INIReader iconTheme( mIconTheme + "index.theme" );
    for( auto fbTh: iconTheme.GetList( "Icon Theme", "Inherits", ',' ) ) {
        for( std::string dir: icoDirs ) {
            if ( exists( dir + fbTh ) ) {
                themes.push_back( dir + fbTh + "/" );
            }
        }
    }

    /* If hicolor is not in the list, add it */
    if ( not std::count( themes.begin(), themes.end(), "/usr/share/icons/hicolor/" ) )
        themes.push_back( "/usr/share/icons/hicolor/" );

    /* Now get the list of theme dirs; clear the list first */
    themeDirs.clear();
    for( auto theme: themes ) {
        INIReader iconTheme( theme + "index.theme" );
        for( auto dir: iconTheme.GetList( "Icon Theme", "Directories", ',' ) ) {
            themeDirs.push_back( theme + dir + "/" );
        }
    }

    themeDirs.push_back( "/usr/share/pixmaps/" );
};

std::string wf::windecor::IconThemeManager::iconPathForAppId( std::string mAppId ) const {

    std::string home_local_share = getenv( "HOME" );
    home_local_share += "/.local/share/applications/";

    std::vector<std::string> appDirs = {
        home_local_share,
        "/usr/local/share/applications/",
        "/usr/share/applications/",
    };

    std::string iconName = INSTALL_PREFIX "/share/wayfire/windecor/resources/executable.svg";
    bool found = false;
    for( auto path: appDirs ) {
        if ( exists( path + mAppId + ".desktop" ) ) {
            INIReader desktop( path + mAppId + ".desktop" );
            iconName = desktop.Get( "Desktop Entry", "Icon", "application-x-executable" );
            if ( not iconName.empty() ) {
                found = true;
                break;
            }
        }
    }

    if ( not found and workHard ) {
        /* Check all desktop files for */
        for( auto path: appDirs ) {
            std::vector<std::string> desktops = getDesktops( path );
            for ( std::string dskf: desktops ) {
                INIReader desktop( dskf );

                /* Check the executable name */
                std::istringstream iss( desktop.Get( "Desktop Entry", "Exec", "abcd1234/" ) );
                std::string exec;
                getline( iss, exec, ' ' );

                if ( std::filesystem::path( exec ).filename() == mAppId ) {
                    iconName = desktop.Get( "Desktop Entry", "Icon", "application-x-executable" );
                    if ( not iconName.empty() )
                        break;
                }

                /* Check StartupWMClass - electron apps set this, Courtesy @wb9688 */
                std::istringstream ess( desktop.Get( "Desktop Entry", "StartupWMClass", "abcd1234/" ) );
                std::string cls;
                getline( ess, cls, ' ' );

                if ( cls == mAppId ) {
                    iconName = desktop.Get( "Desktop Entry", "Icon", "application-x-executable" );
                    if ( not iconName.empty() )
                        break;
                }
            }
        }
    }

    /* In case a full path is specified */
    if ( ( iconName.at( 0 ) == '/' ) and exists( iconName ) ) {
        return iconName;
    }

    std::vector<std::string> iconNames;

    iconNames.push_back( iconName );

    std::string iconNameL = iconName;
    transform( iconNameL.begin(), iconNameL.end(), iconNameL.begin(), ::tolower );
    iconNames.push_back( iconNameL );

    iconNames.push_back( mAppId );

    std::string mAppIdL = mAppId;
    transform( mAppIdL.begin(), mAppIdL.end(), mAppIdL.begin(), ::tolower );
    iconNames.push_back( mAppIdL );

    std::vector<std::string> paths;
    for( auto themeDir: themeDirs ) {
        for( auto name: iconNames ) {
            if ( exists( themeDir + name + ".svg" ) )
                paths.push_back( themeDir + name + ".svg" );

            if ( exists( themeDir + name + ".png" ) )
                paths.push_back( themeDir + name + ".png" );
        }
    }

    std::vector<std::string> filtered;

    /* scalable */
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/scalable" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 512px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/x512" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 256px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/x256" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 128px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/x128" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 96px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/96" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 64px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/64" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 48px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/48" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 36px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/36" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 32px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/32" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 24px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/24" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 22px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/22" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* 16px */
    filtered.clear();
    std::copy_if(
        paths.begin(), paths.end(), std::back_inserter( filtered ), [=] ( std::string path ) {
            return ( path.find( "/16" ) != std::string::npos );
        }
    );
    if  ( not filtered.empty() )
        return filtered.at( 0 );

    /* Return the first in the list */
    if ( not paths.empty() )
        return paths.at( 0 );

    std::string iconPath = INSTALL_PREFIX "/share/wayfire/windecor/executable.svg";
    return iconPath;
};
