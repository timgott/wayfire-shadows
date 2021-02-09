#pragma once

#include <wayfire/option-wrapper.hpp>
#include <iostream>
#include <vector>

namespace wf {
namespace windecor {
    class IconThemeManager {
        public:
            /* Icon Theme Manager for WinDecor */
            IconThemeManager( std::string iconTheme );

            /* In case the theme is changed when the plugin is running */
            void setIconTheme( std::string newTheme );

            /* The path of the icon for an app_id */
            std::string iconPathForAppId( std::string app_id ) const;

        private:
            std::string mIconTheme;
            std::vector<std::string> themeDirs;
            wf::option_wrapper_t<bool> workHard{ "windecor/work_hard" };
    };
}
}
