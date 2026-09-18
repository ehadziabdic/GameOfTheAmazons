cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME AmazonsGame)	#Unijeti naziv solution-a

#BEGIN dio koji ide u sve solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
if (WIN32)
	set(HOME_ROOT $ENV{USERPROFILE})
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
	set(HOME_ROOT $ENV{HOME})
endif()
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)

# --- Diagnostic only: without this, a missing or misplaced SDK produces six
#     cascading "include could not find" / target_link_libraries errors that
#     point at the wrong files. This turns it into one readable message.
if (NOT EXISTS "${NATID_SDK_ROOT}/DevEnv/Common.cmake")
	message(FATAL_ERROR
		"natID SDK not found.\n"
		"  Looked for: ${NATID_SDK_ROOT}/DevEnv/Common.cmake\n"
		"  HOME        = $ENV{HOME}\n"
		"  USERPROFILE = $ENV{USERPROFILE}\n"
		"Install the SDK so that <home folder>/natID.SDK/DevEnv/Common.cmake "
		"exists (see https://github.com/idzafic/natID), then delete this build "
		"folder and configure again.")
endif()

include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END dio koji ide u sve solutions

include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

include(AmazonsGame.cmake)
