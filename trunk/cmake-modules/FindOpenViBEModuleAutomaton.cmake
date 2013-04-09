# ---------------------------------
# Finds module Automaton 
# Adds library to target
# Adds include path
# ---------------------------------
OPTION(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON "Dynamically link OpenViBE module Automaton" ON)

IF(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON)
	ADD_DEFINITIONS(-DAUTOMATON_Shared)
ENDIF(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON)

IF(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON)
	SET(OPENVIBE_MODULE_AUTOMATON_LINKING "")
ELSE(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON)
	SET(OPENVIBE_MODULE_AUTOMATON_LINKING "-static")
ENDIF(DYNAMIC_LINK_OPENVIBE_MODULE_AUTOMATON)

IF(OV_BRANCH_MODULES_AUTOMATON)
	set(SRC_DIR ${OV_BASE_DIR}/openvibe-modules/automaton/${OV_BRANCH_MODULES_AUTOMATON})
ELSE(OV_BRANCH_MODULES_AUTOMATON)
	set(SRC_DIR ${OV_BASE_DIR}/openvibe-modules/automaton/${OV_TRUNK})
ENDIF(OV_BRANCH_MODULES_AUTOMATON)

FIND_PATH(PATH_OPENVIBE_MODULES_AUTOMATON src/automaton/defines.h PATHS ${SRC_DIR})
IF(PATH_OPENVIBE_MODULES_AUTOMATON)
	MESSAGE(STATUS "  Found OpenViBE module Automaton...")
	INCLUDE_DIRECTORIES(${PATH_OPENVIBE_MODULES_AUTOMATON}/src/)

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} openvibe-module-automaton${OPENVIBE_MODULE_AUTOMATON_LINKING})

	ADD_DEFINITIONS(-DTARGET_HAS_Automaton)
ELSE(PATH_OPENVIBE_MODULES_AUTOMATON)
	MESSAGE(STATUS "  FAILED to find OpenViBE module Automaton...")
ENDIF(PATH_OPENVIBE_MODULES_AUTOMATON)
