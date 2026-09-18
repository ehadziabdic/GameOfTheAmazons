cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME AmazonsGame)	#Unijeti naziv solution-a

#BEGIN dio koji ide u sve solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})

# --- Locate the folder that contains natID.SDK.
#     macOS/Linux always define HOME. Windows does not define it at all by
#     default, so fall back to USERPROFILE there.
set(HOME_ROOT "$ENV{HOME}")
if (WIN32)
	if (NOT HOME_ROOT)
		set(HOME_ROOT "$ENV{USERPROFILE}")
	endif()
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()

# --- Some natID setups leave HOME pointing at the SDK folder itself
#     (HOME=<home>/natID.SDK) instead of at the home folder. Recover from that
#     instead of failing with six unreadable include errors.
if (NOT EXISTS "${HOME_ROOT}/natID.SDK/DevEnv/Common.cmake"
		AND EXISTS "${HOME_ROOT}/DevEnv/Common.cmake")
	get_filename_component(HOME_ROOT "${HOME_ROOT}" DIRECTORY)
endif()

# --- Last resort on Windows: HOME is set, but to something that has no SDK.
if (WIN32 AND NOT EXISTS "${HOME_ROOT}/natID.SDK/DevEnv/Common.cmake"
		AND EXISTS "$ENV{USERPROFILE}/natID.SDK/DevEnv/Common.cmake")
	set(HOME_ROOT "$ENV{USERPROFILE}")
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()

set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)

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

message(STATUS "natID SDK: ${NATID_SDK_ROOT}")

include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END dio koji ide u sve solutions

include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

# --- Make Xcode generate a runnable scheme for the app target.
#     Harmless (and ignored) on every other generator.
set(CMAKE_XCODE_GENERATE_SCHEME ON)

include(${CMAKE_CURRENT_LIST_DIR}/AmazonsGame.cmake)
