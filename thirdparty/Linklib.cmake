
set(APP_DEPEND_LIBS ${APP_DEPEND_LIBS} 
    Ws2_32.lib 
    Wldap32.lib 
    Crypt32.lib 
    Normaliz.lib 
    libcurl.lib 
    PARENT_SCOPE)

set(APP_LIB_DIRS ${APP_LIB_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts/lib/${ToolPlatform}
    PARENT_SCOPE)

if(WIN32)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts/lib/${ToolPlatform}/pthreadVC2.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()
