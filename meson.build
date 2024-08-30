project(
	'wayfire-plugin-winshadows',
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
wfconfig = dependency('wf-config')

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])
add_project_arguments(['-DWAYFIRE_PLUGIN'], language: ['cpp', 'c'])
add_project_link_arguments(['-rdynamic'], language:'cpp')

shadows = shared_module(
    'winshadows', [
        'winshadows.cpp',
        'node.cpp',
        'renderer.cpp',
        'shaders.glsl.cpp',
    ],

    dependencies: [
        wfconfig,
        wayfire
    ],

    install: true,
    install_dir: join_paths( get_option( 'libdir' ), 'wayfire' )
)

install_data( 'winshadows.xml', install_dir: wayfire.get_variable( pkgconfig: 'metadatadir' ) )

summary = [
	'',
	'----------------',
	'wayfire-plugin-winshadows @0@'.format( meson.project_version() ),
	'----------------',
	''
]
message('\n'.join(summary))
