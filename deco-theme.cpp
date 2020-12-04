#include "deco-theme.hpp"
#include <wayfire/core.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/config.h>
#include <map>
#include <librsvg/rsvg.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "INIReader.h"

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

/** Create a new theme with the default parameters */
wf::windecor::decoration_theme_t::decoration_theme_t( std::string app_id ) {

    mAppId = app_id;
}

/** @return The available height for displaying the title */
int wf::windecor::decoration_theme_t::get_title_height() const {

    return title_height;
}

/** @return The available border for resizing */
int wf::windecor::decoration_theme_t::get_border_size() const {

    return border_size;
}

/**
 * Fill the given rectange with the background color( s ).
 *
 * @param fb The target framebuffer, must have been bound already
 * @param rectangle The rectangle to redraw.
 * @param scissor The GL scissor rectangle to use.
 * @param active Whether to use active or inactive colors
 */
void wf::windecor::decoration_theme_t::render_background( const wf::framebuffer_t& fb, wf::geometry_t rectangle, const wf::geometry_t& scissor, bool active ) const {

    wf::color_t color = active ? active_color : inactive_color;
    OpenGL::render_begin( fb );
    fb.logic_scissor( scissor );
    OpenGL::render_rectangle( rectangle, color, fb.get_orthographic_projection() );
    OpenGL::render_end();
}

/**
 * Render the given text on a cairo_surface_t with the given size.
 * The caller is responsible for freeing the memory afterwards.
 */
cairo_surface_t* wf::windecor::decoration_theme_t::render_text( std::string text, int width, int height ) const {

    const auto format = CAIRO_FORMAT_ARGB32;
    auto surface = cairo_image_surface_create( format, width, height );
    auto cr = cairo_create( surface );

    const float font_scale = 0.7;
    const float font_size  = height * font_scale;

    // render text
    cairo_select_font_face( cr, ( ( std::string )font ).c_str(),
        CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL );
    cairo_set_source_rgba( cr, 1, 1, 1, 1 );

    cairo_set_font_size( cr, font_size );
    cairo_move_to( cr, 0, font_size );

    cairo_text_extents_t ext;
    cairo_text_extents( cr, text.c_str(), &ext );
    cairo_show_text( cr, text.c_str() );
    cairo_destroy( cr );

    return surface;
}

cairo_surface_t* wf::windecor::decoration_theme_t::get_button_surface( button_type_t button, const button_state_t& state ) const {

    cairo_surface_t *button_surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, state.width, state.height );
    auto cr = cairo_create( button_surface );

    /* Clear the button background */
    cairo_rectangle( cr, 0, 0, state.width, state.height );
    cairo_set_operator( cr, CAIRO_OPERATOR_CLEAR );
    cairo_set_source_rgba( cr, 0, 0, 0, 0 );
    cairo_fill( cr );

    /* Render button itself */
    cairo_set_operator( cr, CAIRO_OPERATOR_OVER );
    cairo_rectangle( cr, 0, 0, state.width, state.height );

    /* Border */
    cairo_set_line_width( cr, state.border );
    cairo_set_source_rgba( cr, 0.0, 0.0, 0.0, 1.0 );
    cairo_stroke_preserve( cr );

    /* Button color */
    wf::color_t base_background = { 0.0, 0.0, 0.0, 0.5 };
    wf::color_t hover_add_background = { 0.0, 0.0, 0.0, 0.5 };
    switch ( button ) {
        case BUTTON_CLOSE:
            base_background = ( wf::color_t )close_color;
            base_background.a = 0.5;
            break;

        case BUTTON_TOGGLE_MAXIMIZE:
            base_background = ( wf::color_t )maximize_color;
            base_background.a = 0.5;
            break;

        case BUTTON_MINIMIZE:
            base_background = ( wf::color_t )minimize_color;
            base_background.a = 0.5;
            break;

        case BUTTON_ICON:
            base_background = { 0.0, 0.0, 0.0, 0.0 };
            hover_add_background = { 0.0, 0.0, 0.0, 0.0 };
            break;

        default:
            assert( false );
    }

    cairo_set_source_rgba( cr,
        base_background.r + hover_add_background.r * state.hover_progress,
        base_background.g + hover_add_background.g * state.hover_progress,
        base_background.b + hover_add_background.b * state.hover_progress,
        base_background.a + hover_add_background.a * state.hover_progress            // All colors will be rendered at 50% intensity
    );
    cairo_fill_preserve( cr );

    /* Icon */
    if ( button == BUTTON_ICON ) {
        std::string iconPath = get_icon_for_app_id();
        cairo_surface_t *button_icon;
        /* Draw svg */
        if ( iconPath.find( ".svg" ) != std::string::npos ) {
            RsvgHandle *svg = rsvg_handle_new_from_file( iconPath.c_str(), 0 );
            RsvgDimensionData size;
            rsvg_handle_get_dimensions( svg, &size );

            /* Render it 4 times bigger and then scale down to get a smooth pix */
            button_icon = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, size.width * 4, size.height * 4 );
            auto crsvg = cairo_create( button_icon );
            cairo_scale( crsvg, 4, 4 );
            rsvg_handle_render_cairo( svg, crsvg );
            cairo_fill( crsvg );
            cairo_destroy( crsvg );
        }

        /* Draw png */
        else {
            button_icon = cairo_image_surface_create_from_png( iconPath.c_str() );
        }

        cairo_scale( cr,
            1.0 * state.width / cairo_image_surface_get_width( button_icon ),
            1.0 * state.height / cairo_image_surface_get_height( button_icon )
        );
        cairo_set_source_surface( cr, button_icon, 0, 0 );
    }

    cairo_fill( cr );
    cairo_destroy( cr );

    return button_surface;
}

std::string wf::windecor::decoration_theme_t::get_icon_for_app_id() const {

    /* First read the icon name from desktop file */
    char home[ 256 ] = {};
    strcpy( home, getenv( "HOME" ) );
    strcat( home, "/.local/share/applications/" );

    std::vector<std::string> appDirs = {
        home,
        "/usr/local/share/applications/",
        "/usr/share/applications/",
    };

    std::string iconName = "application-x-executable";
    for( auto path: appDirs ) {
        if ( exists( path + mAppId + ".desktop" ) ) {
            INIReader desktop( path + mAppId + ".desktop" );
            iconName = desktop.Get( "Desktop Entry", "Icon", "application-x-executable" );
            if ( iconName.size() )
                break;
        }
    }

    /* In case a full path is specified */
    if ( ( iconName.at( 0 ) == '/' ) and exists( iconName ) ) {
        return iconName;
    }

    /* Get the icon path from icon theme */
    std::string icon_theme;
    if ( getenv( "WINDECOR_ICON_THEME" ) )
        icon_theme = getenv( "WINDECOR_ICON_THEME" );

    else
        icon_theme = "";

    std::string hicolor = "/usr/share/icons/hicolor/";
    if ( not icon_theme.size() ) {
        if ( exists( "/usr/share/icons/Adwaita" ) )
            icon_theme = "/usr/share/icons/Adwaita/";

        else if ( exists( "/usr/share/icons/breeze" ) )
            icon_theme = "/usr/share/icons/breeze/";

        else
            icon_theme = hicolor;
    }

    else {
        icon_theme = "/usr/share/icons/" + icon_theme + "/";
    }

    std::vector<std::string> themes = { icon_theme };
    INIReader iconTheme( icon_theme + "index.theme" );
    for( auto fbTh: iconTheme.GetList( "Icon Theme", "Inherits", ',' ) ) {
        if ( exists( "/usr/share/icons/" + fbTh + "/" ) )
            themes.push_back( "/usr/share/icons/" + fbTh + "/" );
    }

    themes.push_back( hicolor );

    std::vector<std::string> paths;
    for( auto theme: themes ) {
        paths.clear();
        INIReader iconTheme( theme + "index.theme" );
        for( auto dir: iconTheme.GetList( "Icon Theme", "Directories", ',' ) ) {
            if ( exists( theme + dir + "/" + iconName + ".svg" ) )
                paths.push_back( theme + dir + "/" + iconName + ".svg" );

            if ( exists( theme + dir + "/" + iconName + ".png" ) )
                paths.push_back( theme + dir + "/" + iconName + ".png" );
        }

        if ( paths.size() ) {
            /* Get the largest possible */
            for( auto path: paths ) {
                /* scalable */
                if ( ( path.find( "/scalable/" ) != std::string::npos ) )
                    return path;
                /* 512px */
                else if ( ( path.find( "/512/" ) != std::string::npos ) or ( path.find( "/512x512/" ) != std::string::npos ) )
                    return path;
                /* 256px */
                else if ( ( path.find( "/256/" ) != std::string::npos ) or ( path.find( "/256x256/" ) != std::string::npos ) )
                    return path;
                /* 128px */
                else if ( ( path.find( "/128/" ) != std::string::npos ) or ( path.find( "/128x128/" ) != std::string::npos ) )
                    return path;
                /* 96px */
                else if ( ( path.find( "/96/" ) != std::string::npos ) or ( path.find( "/96x96/" ) != std::string::npos ) )
                    return path;
                /* 64px */
                else if ( ( path.find( "/64/" ) != std::string::npos ) or ( path.find( "/64x64/" ) != std::string::npos ) )
                    return path;
                /* 48px */
                else if ( ( path.find( "/48/" ) != std::string::npos ) or ( path.find( "/48x48/" ) != std::string::npos ) )
                    return path;
                /* 36px */
                else if ( ( path.find( "/36/" ) != std::string::npos ) or ( path.find( "/36x36/" ) != std::string::npos ) )
                    return path;
                /* 32px */
                else if ( ( path.find( "/32/" ) != std::string::npos ) or ( path.find( "/32x32/" ) != std::string::npos ) )
                    return path;
                /* 24px */
                else if ( ( path.find( "/24/" ) != std::string::npos ) or ( path.find( "/24x24/" ) != std::string::npos ) )
                    return path;
                /* 22px */
                else if ( ( path.find( "/22/" ) != std::string::npos ) or ( path.find( "/22x22/" ) != std::string::npos ) )
                    return path;
                /* 16px */
                else if ( ( path.find( "/16/" ) != std::string::npos ) or ( path.find( "/16x16/" ) != std::string::npos ) )
                    return path;
            }

            /* Return the first in the list */
            return paths.at( 0 );
        }
    }

    std::string iconPath = "/usr/share/icons/breeze/apps/48/fluid.svg";
    return iconPath;
}
