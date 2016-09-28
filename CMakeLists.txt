cmake_minimum_required (VERSION 3.0)
project (Quetoo)

# find arch

include (cmake/TargetArch.cmake)
target_architecture (ARCH)

# general settings

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fPIC")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no-undefined,-allow-multiple-definition")
set (COMMON_C_FLAGS "-pipe -fwrapv -fno-strict-aliasing -fomit-frame-pointer -Wall -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable")
set (OBJ_SRC_DIR "deps/Objectively/Sources")
set (OBJMVC_SRC_DIR "deps/ObjectivelyMVC/Sources")

# libraries

if (UNIX)
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -lm -lrt -ldl")
endif()

if (WIN32 OR CYGWIN)
	set (OPENGL_LIB "opengl32")
	set (GLU_LIB "glu32")
	set (SOCKET_LIB "ws2_32")
elseif (APPLE)
	set (OPENGL_LIB "-framework OpenGL")
	set (SOCKET_LIB "")
else ()
	set (OPENGL_LIB "GL")
	set (GLU_LIB "GLU")
	set (SOCKET_LIB "")
endif ()

# default values

if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if (NOT BUILD_TESTS)
	set (BUILD_TESTS "Off")
endif ()

# build target flags

if (CMAKE_BUILD_TYPE MATCHES Debug)
	set (FINAL_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} ${COMMON_C_FLAGS}")
	set (CMAKE_C_FLAGS_DEBUG ${FINAL_C_FLAGS})
elseif (CMAKE_BUILD_TYPE MATCHES Release)
	set (FINAL_C_FLAGS "${CMAKE_C_FLAGS_RELEASE} ${COMMON_C_FLAGS}")
	set (CMAKE_C_FLAGS_RELEASE ${FINAL_C_FLAGS})
elseif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	set (FINAL_C_FLAGS "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${COMMON_C_FLAGS}")
	set (CMAKE_C_FLAGS_RELWITHDEBINFO ${FINAL_C_FLAGS})
endif ()

# strings

if (WIN32)
	if (MINGW)
		set (BUILD_HOST "Windows.MINGW.${ARCH}")
	else()
		set (BUILD_HOST "Windows.MSVC.${ARCH}")
	endif()
elseif (CYGWIN)
	set (BUILD_HOST "Windows.CYGWIN.${ARCH}")
elseif (APPLE)
	set (BUILD_HOST "Darwin.${ARCH}")
else ()
	set (BUILD_HOST "Unix.${ARCH}")
endif ()

set (QUETOO_VERSION "0.1.0")
set (QUETOO_STRING "Quetoo")
set (QUETOO_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
set (QUETOO_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib/quetoo")
set (QUETOO_DATADIR "${CMAKE_INSTALL_PREFIX}/share/quetoo")

# definitions

add_definitions (
	-DCMAKE_BUILD -DPIC
	-DVERSION="${QUETOO_VERSION}"
	-DBUILD_HOST="${BUILD_HOST}"
	-DPACKAGE_STRING="${QUETOO_STRING}"
	-DBINDIR="${QUETOO_BINDIR}"
	-DPKGLIBDIR="${QUETOO_LIBDIR}"
	-DPKGDATADIR="${QUETOO_DATADIR}"
)

# needed packages

find_package (SDL2 REQUIRED)
find_package (SDL2_mixer REQUIRED)
find_package (SDL2_image REQUIRED)
find_package (SDL2_ttf REQUIRED)
find_package (GLIB2 REQUIRED)
find_package (Ncurses REQUIRED)
find_package (ZLIB REQUIRED)
find_package (PhysFS REQUIRED)
find_package (CURL REQUIRED)
find_package (JPEG REQUIRED)
find_package (Fontconfig REQUIRED)
find_package (Iconv REQUIRED)
find_package (LibXml2)
find_package (Check)

# summary

message (STATUS "")
message (STATUS "--------------------------------------------------------------")
message (STATUS "${QUETOO_STRING} ${QUETOO_VERSION} ${BUILD_HOST}")
message (STATUS "--------------------------------------------------------------")
message (STATUS "Install Dir .......... ${CMAKE_INSTALL_PREFIX}")
message (STATUS "Binary Dir ........... ${QUETOO_BINDIR}")
message (STATUS "Game Module Dir ...... ${QUETOO_LIBDIR}")
message (STATUS "Game Data Dir ........ ${QUETOO_DATADIR}")
message (STATUS "Build Type ........... ${CMAKE_BUILD_TYPE}")
message (STATUS "Build Tests .......... ${BUILD_TESTS}")
message (STATUS "CFLAGS ............... ${FINAL_C_FLAGS}")
message (STATUS "--------------------------------------------------------------")
message (STATUS "")

#############
## minizip ##
#############

add_library (minizip
	deps/minizip/zip.c
	deps/minizip/ioapi.c
)

target_include_directories (minizip PRIVATE deps/minizip)

target_link_libraries (minizip z)

target_compile_definitions (minizip PRIVATE NOCRYPT USE_FILE32API)

#################
## Objectively ##
#################

add_library (objectively
	${OBJ_SRC_DIR}/Objectively/Array.c
	${OBJ_SRC_DIR}/Objectively/Boole.c
	${OBJ_SRC_DIR}/Objectively/Class.c
	${OBJ_SRC_DIR}/Objectively/Condition.c
	${OBJ_SRC_DIR}/Objectively/Data.c
	${OBJ_SRC_DIR}/Objectively/Date.c
	${OBJ_SRC_DIR}/Objectively/DateFormatter.c
	${OBJ_SRC_DIR}/Objectively/Dictionary.c
	${OBJ_SRC_DIR}/Objectively/Enum.c
	${OBJ_SRC_DIR}/Objectively/Error.c
	${OBJ_SRC_DIR}/Objectively/Hash.c
	${OBJ_SRC_DIR}/Objectively/IndexPath.c
	${OBJ_SRC_DIR}/Objectively/IndexSet.c
	${OBJ_SRC_DIR}/Objectively/JSONPath.c
	${OBJ_SRC_DIR}/Objectively/JSONSerialization.c
	${OBJ_SRC_DIR}/Objectively/Locale.c
	${OBJ_SRC_DIR}/Objectively/Lock.c
	${OBJ_SRC_DIR}/Objectively/Log.c
	${OBJ_SRC_DIR}/Objectively/MutableArray.c
	${OBJ_SRC_DIR}/Objectively/MutableData.c
	${OBJ_SRC_DIR}/Objectively/MutableDictionary.c
	${OBJ_SRC_DIR}/Objectively/MutableSet.c
	${OBJ_SRC_DIR}/Objectively/MutableString.c
	${OBJ_SRC_DIR}/Objectively/Null.c
	${OBJ_SRC_DIR}/Objectively/Number.c
	${OBJ_SRC_DIR}/Objectively/NumberFormatter.c
	${OBJ_SRC_DIR}/Objectively/Object.c
	${OBJ_SRC_DIR}/Objectively/Operation.c
	${OBJ_SRC_DIR}/Objectively/OperationQueue.c
	${OBJ_SRC_DIR}/Objectively/Regex.c
	${OBJ_SRC_DIR}/Objectively/Set.c
	${OBJ_SRC_DIR}/Objectively/String.c
	${OBJ_SRC_DIR}/Objectively/Thread.c
	${OBJ_SRC_DIR}/Objectively/URL.c
	${OBJ_SRC_DIR}/Objectively/URLRequest.c
	${OBJ_SRC_DIR}/Objectively/URLSession.c
	${OBJ_SRC_DIR}/Objectively/URLSessionConfiguration.c
	${OBJ_SRC_DIR}/Objectively/URLSessionDataTask.c
	${OBJ_SRC_DIR}/Objectively/URLSessionDownloadTask.c
	${OBJ_SRC_DIR}/Objectively/URLSessionTask.c
	${OBJ_SRC_DIR}/Objectively/URLSessionUploadTask.c
	${OBJ_SRC_DIR}/Objectively/Value.c
)

target_include_directories (objectively PRIVATE
	${OBJ_SRC_DIR}
)

if (MINGW)
	target_link_libraries (objectively regex msvcr110)
endif ()

target_link_libraries (objectively
	${CURL_LIBRARIES}
	${ICONV_LIBRARIES}
)

if (UNIX OR MINGW)
	target_compile_definitions (objectively PRIVATE _GNU_SOURCE)
endif ()

####################
## ObjectivelyMVC ##
####################

add_library (objectivelymvc
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Action.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Box.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Button.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Checkbox.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/CollectionItemView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/CollectionView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Colors.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Control.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Font.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Image.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/ImageView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Input.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Label.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/NavigationViewController.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Option.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Panel.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Renderer.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/ScrollView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Select.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Slider.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/StackView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableCellView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableColumn.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableHeaderCellView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableHeaderView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableRowView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TableView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/Text.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/TextView.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/View.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/ViewController.c
	${OBJMVC_SRC_DIR}/ObjectivelyMVC/WindowController.c
)

target_include_directories (objectivelymvc PRIVATE
	${OBJ_SRC_DIR}
	${OBJMVC_SRC_DIR}
)

if (MINGW)
	target_link_libraries (objectivelymvc mingw32)
endif()

target_link_libraries (objectivelymvc
	objectively
	${SDL2_SDLMAIN_LIBRARY}
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARY}
	${SDL2_TTF_LIBRARIES}
	${OPENGL_LIB}
	${GLU_LIB}
	${FONTCONFIG_LIBRARIES}
)

if (UNIX OR MINGW)
	target_compile_definitions (objectivelymvc PRIVATE _GNU_SOURCE)
endif ()

###########
## cgame ##
###########

add_library (cgame MODULE
	src/cgame/default/cg_client.c
	src/cgame/default/cg_emit.c
	src/cgame/default/cg_effect.c
	src/cgame/default/cg_entity.c
	src/cgame/default/cg_entity_effect.c
	src/cgame/default/cg_entity_event.c
	src/cgame/default/cg_entity_trail.c
	src/cgame/default/cg_hud.c
	src/cgame/default/cg_main.c
	src/cgame/default/cg_media.c
	src/cgame/default/cg_muzzle_flash.c
	src/cgame/default/cg_particle.c
	src/cgame/default/cg_predict.c
	src/cgame/default/cg_score.c
	src/cgame/default/cg_temp_entity.c
	src/cgame/default/cg_view.c
	src/game/default/bg_pmove.c
	src/shared.c
	src/shared-anorms.c
)

target_include_directories (cgame PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	${OBJ_SRC_DIR}
	${OBJMVC_SRC_DIR}
	src
	src/cgame/default
)

target_link_libraries (cgame
	${GLIB2_LIBRARIES}
)

set_target_properties (cgame PROPERTIES PREFIX "")

##########
## game ##
##########

add_library (game MODULE
	src/game/default/g_ai.c
	src/game/default/g_ai_goal.c
	src/game/default/g_ballistics.c
	src/game/default/g_client_chase.c
	src/game/default/g_client_stats.c
	src/game/default/g_client_view.c
	src/game/default/g_client.c
	src/game/default/g_cmd.c
	src/game/default/g_combat.c
	src/game/default/g_entity_func.c
	src/game/default/g_entity_info.c
	src/game/default/g_entity_misc.c
	src/game/default/g_entity_target.c
	src/game/default/g_entity_trigger.c
	src/game/default/g_entity.c
	src/game/default/g_item.c
	src/game/default/g_main.c
	src/game/default/g_map_list.c
	src/game/default/g_mysql.c
	src/game/default/g_physics.c
	src/game/default/g_util.c
	src/game/default/g_weapon.c
	src/game/default/bg_pmove.c
	src/shared.c
	src/shared-anorms.c
)

target_include_directories (game PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	src
	src/cgame/default
)

target_link_libraries (game
	${SOCKET_LIB}
	${GLIB2_LIBRARIES}
)

set_target_properties (game PROPERTIES PREFIX "")

############
## quetoo ##
############

add_executable (quetoo
	src/main/main.c
	src/server/sv_admin.c
	src/server/sv_client.c
	src/server/sv_console.c
	src/server/sv_entity.c
	src/server/sv_game.c
	src/server/sv_init.c
	src/server/sv_main.c
	src/server/sv_master.c
	src/server/sv_send.c
	src/server/sv_world.c
	src/client/cl_cgame.c
	src/client/cl_cmd.c
	src/client/cl_console.c
	src/client/cl_demo.c
	src/client/cl_entity.c
	src/client/cl_http.c
	src/client/cl_input.c
	src/client/cl_keys.c
	src/client/cl_main.c
	src/client/cl_media.c
	src/client/cl_mouse.c
	src/client/cl_parse.c
	src/client/cl_predict.c
	src/client/cl_screen.c
	src/client/cl_server.c
	src/client/cl_view.c
	src/client/renderer/r_array.c
	src/client/renderer/r_bsp_light.c
	src/client/renderer/r_bsp_model.c
	src/client/renderer/r_bsp_surface.c
	src/client/renderer/r_bsp.c
	src/client/renderer/r_context.c
	src/client/renderer/r_corona.c
	src/client/renderer/r_draw.c
	src/client/renderer/r_element.c
	src/client/renderer/r_entity.c
	src/client/renderer/r_flare.c
	src/client/renderer/r_gl.c
	src/client/renderer/r_image.c
	src/client/renderer/r_light.c
	src/client/renderer/r_lighting.c
	src/client/renderer/r_lightmap.c
	src/client/renderer/r_main.c
	src/client/renderer/r_material.c
	src/client/renderer/r_media.c
	src/client/renderer/r_mesh_model.c
	src/client/renderer/r_mesh_shadow.c
	src/client/renderer/r_mesh_shell.c
	src/client/renderer/r_mesh.c
	src/client/renderer/r_model.c
	src/client/renderer/r_particle.c
	src/client/renderer/r_program.c
	src/client/renderer/r_program_default.c
	src/client/renderer/r_program_shadow.c
	src/client/renderer/r_program_shell.c
	src/client/renderer/r_program_warp.c
	src/client/renderer/r_sky.c
	src/client/renderer/r_state.c
	src/client/sound/s_main.c
	src/client/sound/s_media.c
	src/client/sound/s_mix.c
	src/client/sound/s_music.c
	src/client/sound/s_sample.c
	src/client/ui/ui_data.c
	src/client/ui/ui_main.c
	src/client/ui/viewcontrollers/CreateServerViewController.c
	src/client/ui/viewcontrollers/KeysViewController.c
	src/client/ui/viewcontrollers/MainViewController.c
	src/client/ui/viewcontrollers/MenuViewController.c
	src/client/ui/viewcontrollers/MouseViewController.c
	src/client/ui/viewcontrollers/MultiplayerViewController.c
	src/client/ui/viewcontrollers/PlayerViewController.c
	src/client/ui/viewcontrollers/SystemViewController.c
	src/client/ui/views/BindTextView.c
	src/client/ui/views/CrosshairView.c
	src/client/ui/views/CvarCheckbox.c
	src/client/ui/views/CvarSelect.c
	src/client/ui/views/CvarSlider.c
	src/client/ui/views/CvarTextView.c
	src/client/ui/views/MapListCollectionItemView.c
	src/client/ui/views/MapListCollectionView.c
	src/client/ui/views/PlayerModelView.c
	src/client/ui/views/PrimaryButton.c
	src/client/ui/views/ServersTableView.c
	src/client/ui/views/SkinSelect.c
	src/client/ui/views/VideoModeSelect.c
	src/collision/cm_model.c
	src/collision/cm_test.c
	src/collision/cm_trace.c
	src/collision/cm_vis.c
	src/net/net.c
	src/net/net_chan.c
	src/net/net_message.c
	src/net/net_tcp.c
	src/net/net_udp.c
	src/cmd.c
	src/console.c
	src/cvar.c
	src/filesystem.c
	src/matrix.c
	src/mem.c
	src/mem_buf.c
	src/swap.c
	src/sys.c
	src/thread.c
	src/shared.c
	src/shared-anorms.c
	src/common.c
	src/image.c
	src/main/quetoo-icon.rc
)

target_include_directories (quetoo PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	${NCURSES_INCLUDE_DIRS}
	${OBJ_SRC_DIR}
	${OBJMVC_SRC_DIR}
	src
	src/net
	src/collision
	src/client
	src/client/ui
	src/client/ui/views
)

if (MINGW)
	target_link_libraries (quetoo mingw32)
endif()

target_link_libraries (quetoo
	${SOCKET_LIB}
	${SDL2_SDLMAIN_LIBRARY}
	${SDL2_LIBRARY}
	${SDLMIXER_LIBRARY}
	${GLIB2_LIBRARIES}
	${JPEG_LIBRARY}
	${PHYSFS_LIBRARY}
	${NCURSES_LIBRARIES}
	objectivelymvc
)

######################
## quetoo-dedicated ##
######################

add_executable (quetoo-dedicated
	src/main/main.c
	src/server/sv_admin.c
	src/server/sv_client.c
	src/server/sv_console.c
	src/server/sv_entity.c
	src/server/sv_game.c
	src/server/sv_init.c
	src/server/sv_main.c
	src/server/sv_master.c
	src/server/sv_send.c
	src/server/sv_world.c
	src/client/cl_null.c
	src/collision/cm_model.c
	src/collision/cm_test.c
	src/collision/cm_trace.c
	src/collision/cm_vis.c
	src/net/net.c
	src/net/net_chan.c
	src/net/net_message.c
	src/net/net_tcp.c
	src/net/net_udp.c
	src/cmd.c
	src/console.c
	src/cvar.c
	src/filesystem.c
	src/matrix.c
	src/mem.c
	src/mem_buf.c
	src/swap.c
	src/sys.c
	src/thread.c
	src/shared.c
	src/shared-anorms.c
	src/common.c
	src/main/quetoo-dedicated-icon.rc
)

target_include_directories (quetoo-dedicated PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	${NCURSES_INCLUDE_DIRS}
	${OBJ_SRC_DIR}
	${OBJMVC_SRC_DIR}
	src
	src/net
	src/collision
)

if (MINGW)
	target_link_libraries (quetoo-dedicated mingw32)
endif()

target_link_libraries (quetoo-dedicated
	${SOCKET_LIB}
	${SDL2_SDLMAIN_LIBRARY}
	${SDL2_LIBRARY}
	${GLIB2_LIBRARIES}
	${PHYSFS_LIBRARY}
	${NCURSES_LIBRARIES}
)

############
## quemap ##
############

add_executable (quemap
	src/tools/quemap/brush.c
	src/tools/quemap/bspfile.c
	src/tools/quemap/csg.c
	src/tools/quemap/faces.c
	src/tools/quemap/flow.c
	src/tools/quemap/leakfile.c
	src/tools/quemap/lightmap.c
	src/tools/quemap/main.c
	src/tools/quemap/map.c
	src/tools/quemap/monitor.c
	src/tools/quemap/patches.c
	src/tools/quemap/polylib.c
	src/tools/quemap/portals.c
	src/tools/quemap/prtfile.c
	src/tools/quemap/qaas.c
	src/tools/quemap/qbsp.c
	src/tools/quemap/qmat.c
	src/tools/quemap/qlight.c
	src/tools/quemap/qvis.c
	src/tools/quemap/qzip.c
	src/tools/quemap/scriplib.c
	src/tools/quemap/textures.c
	src/tools/quemap/threads.c
	src/tools/quemap/tree.c
	src/tools/quemap/writebsp.c
	src/common.c
	src/shared.c
	src/shared-anorms.c
	src/mem.c
	src/mem_buf.c
	src/thread.c
	src/filesystem.c
	src/sys.c
	src/swap.c
	src/matrix.c
	src/console.c
	src/cvar.c
	src/cmd.c
	src/image.c
	src/net/net.c
	src/net/net_message.c
	src/net/net_tcp.c
	src/collision/cm_model.c
	src/collision/cm_test.c
	src/collision/cm_trace.c
	src/collision/cm_vis.c
	src/tools/quemap/quemap-icon.rc
)

target_include_directories (quemap PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	${NCURSES_INCLUDE_DIRS}
	${LIBXML2_INCLUDE_DIR}
	src
	src/tools/quemap
)

if (MINGW)
	target_link_libraries (quemap mingw32)
endif()

target_link_libraries (quemap
	${SOCKET_LIB}
	${SDL2_SDLMAIN_LIBRARY}
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARY}
	${GLIB2_LIBRARIES}
	${NCURSES_LIBRARIES}
	${JPEG_LIBRARY}
	${LIBXML2_LIBRARIES}
	${PHYSFS_LIBRARY}
	minizip
)

###################
## quetoo-update ##
###################

add_executable (quetoo-update
	src/tools/update/main.c
	src/tools/update/update-icon.rc
)

target_include_directories (quetoo-update PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${GLIB2_INCLUDE_DIRS}
	src
	src/tools/update
)

# tests

if (BUILD_TESTS MATCHES "On")
	##############
	## libtests ##
	##############

	add_library (libtests STATIC
		src/tests/tests.c
		src/common.c
		src/shared.c
		src/shared-anorms.c
	)

	target_include_directories (libtests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	###############
	## check_cmd ##
	###############

	add_executable (check_cmd
		src/tests/check_cmd.c
		src/console.c
		src/cmd.c
		src/cvar.c
		src/filesystem.c
		src/mem.c
		src/mem_buf.c
		src/sys.c
		src/swap.c
		src/common.c
		src/shared.c
		src/shared-anorms.c
	)

	target_include_directories (check_cmd PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_cmd mingw32)
	endif()

	target_link_libraries (check_cmd
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${PHYSFS_LIBRARY}
		${CHECK_LIBRARIES}
		libtests
	)

	################
	## check_cvar ##
	################

	add_executable (check_cvar
		src/tests/check_cvar.c
		src/console.c
		src/cmd.c
		src/cvar.c
		src/filesystem.c
		src/mem.c
		src/mem_buf.c
		src/sys.c
		src/swap.c
		src/common.c
		src/shared.c
		src/shared-anorms.c
	)

	target_include_directories (check_cvar PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_cvar mingw32)
	endif()

	target_link_libraries (check_cvar
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${PHYSFS_LIBRARY}
		${CHECK_LIBRARIES}
		libtests
	)

	######################
	## check_filesystem ##
	######################

	add_executable (check_filesystem
		src/tests/check_filesystem.c
		src/filesystem.c
		src/mem.c
		src/mem_buf.c
		src/sys.c
		src/swap.c
		src/common.c
		src/shared.c
		src/shared-anorms.c
	)

	target_include_directories (check_filesystem PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_filesystem mingw32)
	endif()

	target_link_libraries (check_filesystem
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${PHYSFS_LIBRARY}
		${CHECK_LIBRARIES}
		libtests
	)

	##################
	## check_master ##
	##################

	add_executable (check_master
		src/tests/check_master.c
		src/filesystem.c
		src/mem.c
		src/mem_buf.c
		src/sys.c
		src/swap.c
		src/common.c
		src/shared.c
		src/shared-anorms.c
	)

	target_include_directories (check_master PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_master mingw32)
	endif()

	target_link_libraries (check_master
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${PHYSFS_LIBRARY}
		${CHECK_LIBRARIES}
		libtests
	)

	###############
	## check_mem ##
	###############

	add_executable (check_mem
		src/tests/check_mem.c
		src/mem.c
		src/mem_buf.c
	)

	target_include_directories (check_mem PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_mem mingw32)
	endif()

	target_link_libraries (check_mem
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${CHECK_LIBRARIES}
		libtests
	)

	###################
	## check_r_media ##
	###################

	add_executable (check_r_media
		src/tests/check_r_media.c
		src/mem.c
		src/mem_buf.c
	)

	target_include_directories (check_r_media PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_r_media mingw32)
	endif()

	target_link_libraries (check_r_media
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${CHECK_LIBRARIES}
		libtests
	)

	##################
	## check_thread ##
	##################

	add_executable (check_thread
		src/tests/check_thread.c
		src/thread.c
		src/mem.c
		src/mem_buf.c
	)

	target_include_directories (check_thread PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${GLIB2_INCLUDE_DIRS}
		src
		src/tests
	)

	if (MINGW)
		target_link_libraries (check_thread mingw32)
	endif()

	target_link_libraries (check_thread
		${SDL2_SDLMAIN_LIBRARY}
		${SDL2_LIBRARY}
		${GLIB2_LIBRARIES}
		${CHECK_LIBRARIES}
		libtests
	)

endif ()

# install

install (TARGETS
	quetoo
	quetoo-dedicated
	quetoo-update
	quemap
	DESTINATION bin
)

install (TARGETS cgame game LIBRARY DESTINATION "${QUETOO_LIBDIR}/default")

install (FILES
	src/client/renderer/shaders/default_fs.glsl
	src/client/renderer/shaders/default_vs.glsl
	src/client/renderer/shaders/shadow_fs.glsl
	src/client/renderer/shaders/shadow_vs.glsl
	src/client/renderer/shaders/shell_fs.glsl
	src/client/renderer/shaders/shell_vs.glsl
	src/client/renderer/shaders/warp_fs.glsl
	src/client/renderer/shaders/warp_vs.glsl
	DESTINATION "${QUETOO_LIBDIR}/default/shaders"
)

if (BUILD_TESTS MATCHES ON)
	install (TARGETS
		check_cmd
		check_cvar
		check_filesystem
		check_master
		check_mem
		check_r_media
		check_thread
		DESTINATION bin
	)
endif ()