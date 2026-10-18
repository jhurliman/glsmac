# Portable Linux releases must not acquire the build host's libav*.so ABI or its
# hardware codec dependency closure. Build only the original movies' CPU codecs.
INCLUDE( ExternalProject )
FIND_PROGRAM( MOVIE_MAKE NAMES gmake make )
IF ( NOT MOVIE_MAKE )
	MESSAGE( FATAL_ERROR "make is required to build portable movie codecs" )
ENDIF ()
SET( MOVIE_FFMPEG_VERSION "8.1.3" )
SET( MOVIE_FFMPEG_SHA256 "7138d28c96d9d3e3af4ee3d8cad72741f8ffb40da90c1112235dea3ecd3178a3" )
SET( MOVIE_FFMPEG_BASE "${CMAKE_CURRENT_BINARY_DIR}/movie-ffmpeg" )
SET( MOVIE_FFMPEG_PREFIX "${MOVIE_FFMPEG_BASE}/install" )
SET( MOVIE_FFMPEG_URL "https://ffmpeg.org/releases/ffmpeg-${MOVIE_FFMPEG_VERSION}.tar.xz" )
SET( MOVIE_FFMPEG_FLAGS
	--disable-programs --disable-doc --disable-network --disable-autodetect
	--disable-everything --disable-avdevice --disable-avfilter --disable-swresample
	--enable-avformat --enable-avcodec --enable-avutil --enable-swscale
	--enable-protocol=file --enable-demuxer=ea --enable-decoder=eatqi --enable-decoder=adpcm_ea
	--enable-static --disable-shared --enable-pic --disable-x86asm --enable-small )
IF ( CMAKE_BUILD_TYPE STREQUAL "Portable32" )
	LIST( APPEND MOVIE_FFMPEG_FLAGS --arch=x86 --extra-cflags=-m32 --extra-ldflags=-m32 )
ENDIF ()
EXTERNALPROJECT_ADD( glsmac_movie_ffmpeg
	URL "${MOVIE_FFMPEG_URL}" URL_HASH "SHA256=${MOVIE_FFMPEG_SHA256}"
	DOWNLOAD_DIR "${MOVIE_FFMPEG_BASE}/download"
	SOURCE_DIR "${MOVIE_FFMPEG_BASE}/source"
	BINARY_DIR "${MOVIE_FFMPEG_BASE}/build"
	INSTALL_DIR "${MOVIE_FFMPEG_PREFIX}"
	UPDATE_COMMAND ""
	CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> ${MOVIE_FFMPEG_FLAGS}
	BUILD_COMMAND "${MOVIE_MAKE}" -j4
	INSTALL_COMMAND "${MOVIE_MAKE}" install
	BUILD_BYPRODUCTS "${MOVIE_FFMPEG_PREFIX}/lib/libavformat.a"
		"${MOVIE_FFMPEG_PREFIX}/lib/libavcodec.a" "${MOVIE_FFMPEG_PREFIX}/lib/libavutil.a"
		"${MOVIE_FFMPEG_PREFIX}/lib/libswscale.a" )
FILE( MAKE_DIRECTORY "${MOVIE_FFMPEG_PREFIX}/include" )
ADD_DEPENDENCIES( ${PROJECT_NAME} glsmac_movie_ffmpeg )
TARGET_INCLUDE_DIRECTORIES( ${PROJECT_NAME} SYSTEM PRIVATE "${MOVIE_FFMPEG_PREFIX}/include" )
FOREACH ( LIB avformat avcodec swscale avutil )
	ADD_LIBRARY( glsmac_movie_${LIB} STATIC IMPORTED )
	SET_TARGET_PROPERTIES( glsmac_movie_${LIB} PROPERTIES IMPORTED_LOCATION "${MOVIE_FFMPEG_PREFIX}/lib/lib${LIB}.a" )
	TARGET_LINK_LIBRARIES( ${PROJECT_NAME} PRIVATE glsmac_movie_${LIB} )
ENDFOREACH ()
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} PRIVATE m )
STRING( REPLACE ";" " " MOVIE_FFMPEG_COMMAND "${MOVIE_FFMPEG_FLAGS}" )
FILE( WRITE "${MOVIE_FFMPEG_BASE}/SOURCE.txt"
	"FFmpeg ${MOVIE_FFMPEG_VERSION}\nSource: ${MOVIE_FFMPEG_URL}\nSHA256: ${MOVIE_FFMPEG_SHA256}\nConfigure: ${MOVIE_FFMPEG_COMMAND}\nThe matching source archive is included as FFmpeg-source.tar.xz.\nBuild recipe: GLSMAC cmake/PortableMovieFFmpeg.cmake\nGLSMAC source: https://github.com/jhurliman/glsmac\n" )
ADD_CUSTOM_COMMAND( TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOVIE_FFMPEG_BASE}/source/COPYING.LGPLv2.1" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/FFmpeg-LICENSE.txt"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOVIE_FFMPEG_BASE}/SOURCE.txt" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/FFmpeg-SOURCE.txt"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MOVIE_FFMPEG_BASE}/download/ffmpeg-${MOVIE_FFMPEG_VERSION}.tar.xz" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/FFmpeg-source.tar.xz" )
