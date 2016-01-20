
IF(WIN32)
	FIND_LIBRARY(LIB_STANDARD_MODULE_WINHTTP winhttp)
	IF(LIB_STANDARD_MODULE_WINHTTP)
		MESSAGE(STATUS "  Found winhttp...")
		IF(TARGET ${PROJECT_NAME})
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_WINHTTP})
		ENDIF()
		IF(TARGET ${PROJECT_NAME}-static)
			TARGET_LINK_LIBRARIES(${PROJECT_NAME}-static ${LIB_STANDARD_MODULE_WINHTTP})
		ENDIF()
	ELSE(LIB_STANDARD_MODULE_WINHTTP)
		MESSAGE(STATUS "  FAILED to find winhttp...")
	ENDIF(LIB_STANDARD_MODULE_WINHTTP)
ENDIF()

