project(
	'wayfire-plugins-marcus',
    'c',
	'cpp',
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.51.0',
	default_options: [
		'cpp_std=c++17',
        'c_std=c11',
		'warning_level=2',
		'werror=false',
	],
)

wayfire  = dependency('wayfire')
pixman   = dependency('pixman-1')
wlroots  = dependency('wlroots')
wfconfig = dependency('wf-config')
cairo    = dependency('cairo')
rsvg     = dependency( 'librsvg-2.0' )

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])
add_project_arguments(['-DWAYFIRE_PLUGIN'], language: ['cpp', 'c'])
add_project_link_arguments(['-rdynamic'], language:'cpp')


decoration = shared_module(
    'windecor', [
        'decoration.cpp',
        'deco-subsurface.cpp',
        'deco-button.cpp',
        'deco-layout.cpp',
        'deco-theme.cpp'
    ],

    dependencies: [
        wlroots,
        wfconfig,
        pixman,
        cairo,
        rsvg
    ],

    install: true,
    install_dir: join_paths( get_option( 'libdir' ), 'wayfire' )
)

install_data( 'windecor.xml', install_dir: wayfire.get_variable( pkgconfig: 'metadatadir' ) )

summary = [
	'',
	'----------------',
	'wayfire-plugin-windecor @0@'.format(meson.project_version()),
	'----------------',
	''
]
message('\n'.join(summary))
