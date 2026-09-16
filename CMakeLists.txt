cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME AmazonsGame)	#Unijeti naziv solution-a

#BEGIN dio koji ide u sve solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(HOME_ROOT $ENV{HOME})
if (WIN32)
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
    set(HOME_ROOT $ENV{HOME})
endif()
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END dio koji ide u sve solutions

include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

include(AmazonsGame.cmake)


