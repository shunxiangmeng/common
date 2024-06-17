
remove_definitions(-DLOG_LIB_MODULE)

file(GLOB_RECURSE INFRA_SOURCE_FILES jsoncpp/*.cpp)
file(GLOB_RECURSE INFRA_SOURCE_HEADER_FILES jsoncpp/*.h msgpack/*.h msgpack/*.hpp freetype/*.h)

file(GLOB_RECURSE FREETYPE_SOURCE_C freetype-2.13.0/src/autofit/autofit.c 
                    freetype-2.13.0/src/base/ftbase.c
                    freetype-2.13.0/src/base/ftbbox.c
                    freetype-2.13.0/src/base/ftbdf.c
                    freetype-2.13.0/src/base/ftbitmap.c
                    freetype-2.13.0/src/base/ftcid.c
                    freetype-2.13.0/src/base/ftdebug.c
                    freetype-2.13.0/src/base/ftfstype.c
                    freetype-2.13.0/src/base/ftgasp.c
                    freetype-2.13.0/src/base/ftglyph.c
                    freetype-2.13.0/src/base/ftgxval.c
                    freetype-2.13.0/src/base/ftinit.c
                    freetype-2.13.0/src/base/ftmm.c
                    freetype-2.13.0/src/base/ftotval.c
                    freetype-2.13.0/src/base/ftpatent.c
                    freetype-2.13.0/src/base/ftpfr.c
                    freetype-2.13.0/src/base/ftstroke.c
                    freetype-2.13.0/src/base/ftsynth.c
                    freetype-2.13.0/src/base/ftsystem.c
                    freetype-2.13.0/src/base/fttype1.c
                    freetype-2.13.0/src/base/ftwinfnt.c
                    freetype-2.13.0/src/bdf/bdf.c
                    freetype-2.13.0/src/bzip2/ftbzip2.c
                    freetype-2.13.0/src/cache/ftcache.c
                    freetype-2.13.0/src/cff/cff.c
                    freetype-2.13.0/src/cid/type1cid.c
                    freetype-2.13.0/src/gzip/ftgzip.c
                    freetype-2.13.0/src/lzw/ftlzw.c
                    freetype-2.13.0/src/pcf/pcf.c
                    freetype-2.13.0/src/pfr/pfr.c
                    freetype-2.13.0/src/psaux/psaux.c
                    freetype-2.13.0/src/pshinter/pshinter.c
                    freetype-2.13.0/src/psnames/psmodule.c
                    freetype-2.13.0/src/raster/raster.c
                    freetype-2.13.0/src/sdf/sdf.c
                    freetype-2.13.0/src/sfnt/sfnt.c
                    freetype-2.13.0/src/smooth/smooth.c
                    freetype-2.13.0/src/svg/svg.c
                    freetype-2.13.0/src/truetype/truetype.c
                    freetype-2.13.0/src/type1/type1.c
                    freetype-2.13.0/src/type42/type42.c
                    freetype-2.13.0/src/winfonts/winfnt.c)


set(APP_SOURCE_FILES ${APP_SOURCE_FILES} ${INFRA_SOURCE_FILES} ${FREETYPE_SOURCE_C} PARENT_SCOPE)
set(APP_SOURCE_HEADER_FILES ${APP_SOURCE_HEADER_FILES} ${INFRA_SOURCE_HEADER_FILES} PARENT_SCOPE)

set(APP_INCLUDE_DIRS ${APP_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.13.0/include
    PARENT_SCOPE)

add_definitions(-DLOG_LIB_MODULE="thirdparty")
