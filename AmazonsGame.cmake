# --- Application name
set(AMAZONS_NAME AmazonsGame)

# --- Gather sources
file(GLOB AMAZONS_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB AMAZONS_INCS     ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
file(GLOB AMAZONS_INC_TD   ${MY_INC}/td/*.h)
file(GLOB AMAZONS_INC_GUI  ${MY_INC}/gui/*.h)

file(GLOB AMAZONS_INC_THREAD  ${MY_INC}/thread/*.h)
file(GLOB AMAZONS_INC_CNT  ${MY_INC}/cnt/*.h)
file(GLOB AMAZONS_INC_FO  ${MY_INC}/fo/*.h)
file(GLOB AMAZONS_INC_XML  ${MY_INC}/xml/*.h)

# --- Application icon
set(AMAZONS_PLIST  ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/AppIcon.plist)
if(WIN32)
	set(AMAZONS_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
else()
	set(AMAZONS_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
endif()

# --- Executable
add_executable(${AMAZONS_NAME}
    ${AMAZONS_INCS}
    ${AMAZONS_SOURCES}
    ${AMAZONS_INC_TD}
    ${AMAZONS_INC_THREAD}
    ${AMAZONS_INC_CNT} 
    ${AMAZONS_INC_FO} 
    ${AMAZONS_INC_GUI} 
    ${AMAZONS_INC_XML} 
    ${AMAZONS_WINAPP_ICON}
)

# --- Group Sources
source_group("inc"            FILES ${AMAZONS_INCS})
source_group("inc\\td"        FILES ${AMAZONS_INC_TD})
source_group("inc\\cnt"        FILES ${AMAZONS_INC_CNT})
source_group("inc\\fo"        FILES ${AMAZONS_INC_FO})
source_group("inc\\gui"        FILES ${AMAZONS_INC_GUI})
source_group("inc\\thread"        FILES ${AMAZONS_INC_THREAD})
source_group("inc\\xml"        FILES ${AMAZONS_INC_XML})
source_group("src"            FILES ${AMAZONS_SOURCES})

# --- Link natID / mu libraries
target_link_libraries(${AMAZONS_NAME} 
    debug ${MU_LIB_DEBUG} 
    debug ${NATGUI_LIB_DEBUG}
    optimized ${MU_LIB_RELEASE} 
    optimized ${NATGUI_LIB_RELEASE}
)

# --- Apply macros
setTargetPropertiesForGUIApp(${AMAZONS_NAME} ${AMAZONS_PLIST})
setAppIcon(${AMAZONS_NAME} ${CMAKE_CURRENT_LIST_DIR})
setIDEPropertiesForGUIExecutable(${AMAZONS_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${AMAZONS_NAME})