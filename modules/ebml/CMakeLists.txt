PROJECT(openvibe-module-ebml)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h)
INCLUDE_DIRECTORIES(include)
INCLUDE("FindSourceRCProperties")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DEBML_Shared -DEBML_Exports")

ADD_LIBRARY(${PROJECT_NAME}-static STATIC ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME}-static PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DEBML_Static -DEBML_Exports")

IF(UNIX)
	SET_TARGET_PROPERTIES(${PROJECT_NAME}-static PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF(UNIX)

INCLUDE("FindOpenViBECommon")

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
SET_BUILD_PLATFORM()


# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})
INSTALL(TARGETS ${PROJECT_NAME}-static
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})
INSTALL(DIRECTORY include/ DESTINATION ${DIST_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
