# ---------------------------------
# Finds OpenViBE
# Adds library to target
# Adds include path
# ---------------------------------
OPTION(DYNAMIC_LINK_OPENVIBE "Dynamically link OpenViBE" ON)

IF(DYNAMIC_LINK_OPENVIBE)
	ADD_DEFINITIONS(-DOV_Shared)
ENDIF(DYNAMIC_LINK_OPENVIBE)

IF(DYNAMIC_LINK_OPENVIBE)
	SET(OPENVIBE_LINKING "")
ELSE(DYNAMIC_LINK_OPENVIBE)
	SET(OPENVIBE_LINKING "-static")
ENDIF(DYNAMIC_LINK_OPENVIBE)

set(SRC_DIR ${OV_BASE_DIR}/openvibe)

SET(PATH_OPENVIBE "PATH_OPENVIBE-NOTFOUND")
FIND_PATH(PATH_OPENVIBE include/openvibe/ov_all.h PATHS ${SRC_DIR} NO_DEFAULT_PATH)
IF(PATH_OPENVIBE)
	MESSAGE(STATUS "  Found OpenViBE... [${PATH_OPENVIBE}]")
	INCLUDE_DIRECTORIES(${PATH_OPENVIBE}/include/)

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} openvibe${OPENVIBE_LINKING})
	
	ADD_DEFINITIONS(-DTARGET_HAS_OpenViBE)
ELSE(PATH_OPENVIBE)
	MESSAGE(STATUS "  FAILED to find OpenViBE...")
ENDIF(PATH_OPENVIBE)


