# ---------------------------------
# Finds Ogre3D toolkit
#
# Sets Ogre3D_FOUND
# Sets Ogre3D_LIBRARIES
# Sets Ogre3D_LIBRARY_DIRS
# Sets Ogre3D_LDFLAGS
# Sets Ogre3D_LDFLAGS_OTHERS
# Sets Ogre3D_INCLUDE_DIRS
# Sets Ogre3D_CFLAGS
# Sets Ogre3D_CFLAGS_OTHERS
#
# Adds library to target
# Adds include path
# ---------------------------------

IF(WIN32)
	FIND_PATH(PATH_Ogre3D include/OMKBase.h PATHS $ENV{OpenViBE_dependencies} $ENV{OGRE_HOME})
	IF(PATH_Ogre3D)
		SET(Ogre3D_FOUND TRUE)
		SET(Ogre3D_INCLUDE_DIRS ${PATH_Ogre3D}/include)
		IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
			SET(Ogre3D_LIBRARIES OgreMain_d)
		ELSE(CMAKE_BUILD_TYPE STREQUAL "Debug")
			SET(Ogre3D_LIBRARIES OgreMain)
		ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
		SET(Ogre3D_LIBRARY_DIRS ${PATH_Ogre3D}/lib )
	ENDIF(PATH_Ogre3D)
ENDIF(WIN32)

IF(UNIX)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(Ogre3D OGRE)
ENDIF(UNIX)

IF(Ogre3D_FOUND)
	MESSAGE(STATUS "  Found Ogre3D...")
	INCLUDE_DIRECTORIES(${Ogre3D_INCLUDE_DIRS})
	ADD_DEFINITIONS(${Ogre3D_CFLAGS})
	ADD_DEFINITIONS(${Ogre3D_CFLAGS_OTHERS})
	LINK_DIRECTORIES(${Ogre3D_LIBRARY_DIRS})
	FOREACH(Ogre3D_LIB ${Ogre3D_LIBRARIES})
		SET(Ogre3D_LIB1 "Ogre3D_LIB1-NOTFOUND")
		FIND_LIBRARY(Ogre3D_LIB1 NAMES ${Ogre3D_LIB} PATHS ${Ogre3D_LIBRARY_DIRS} ${Ogre3D_LIBDIR} NO_DEFAULT_PATH)
		FIND_LIBRARY(Ogre3D_LIB1 NAMES ${Ogre3D_LIB})
		IF(Ogre3D_LIB1)
			MESSAGE(STATUS "    [  OK  ] Third party lib ${Ogre3D_LIB1}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME}-dynamic ${Ogre3D_LIB1})
		ELSE(Ogre3D_LIB1)
			MESSAGE(STATUS "    [FAILED] Third party lib ${Ogre3D_LIB}")
		ENDIF(Ogre3D_LIB1)
	ENDFOREACH(Ogre3D_LIB)

	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyOgre3D)
ELSE(Ogre3D_FOUND)
	MESSAGE(STATUS "  FAILED to find Ogre3D...")
ENDIF(Ogre3D_FOUND)
