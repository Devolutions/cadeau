
set(MODULE_NAME "xpp")
set(MODULE_PREFIX "XPP")

include_directories(include)

set(${MODULE_PREFIX}_HEADERS "")

set(${MODULE_PREFIX}_SOURCES
	XppColor.c
	XppCopy.c
	XppCompare.c
	XppScale.c
	XppSimd.c
	XppPrimitive.c)

if(WITH_HALIDE)
	add_subdirectory(halide)
	add_definitions(-DWITH_HALIDE)
	include_directories("${HALIDE_INCLUDE_DIR}")
	include_directories("${CMAKE_BINARY_DIR}/generated")
	list(APPEND ${MODULE_PREFIX}_HEADERS ${HALIDE_GENERATED_HEADERS})
	list(APPEND ${MODULE_PREFIX}_SOURCES ${HALIDE_GENERATED_OBJECTS})
	set_source_files_properties(${HALIDE_GENERATED_HEADERS} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${HALIDE_GENERATED_OBJECTS} PROPERTIES GENERATED TRUE)
endif()

if(WITH_SIMD)
	add_definitions(-DWITH_SIMD)

	list(APPEND ${MODULE_PREFIX}_SIMD_SOURCES
		simd/simd.c)
	list(APPEND ${MODULE_PREFIX}_SIMD_HEADERS
		simd/simd.h)
		
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		set_source_files_properties(${${MODULE_PREFIX}_SIMD_SOURCES} PROPERTIES COMPILE_FLAGS "-mfpu=neon")
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")

	else()
		if(MSVC)
			set_source_files_properties(${${MODULE_PREFIX}_SIMD_SOURCES} PROPERTIES COMPILE_FLAGS "/arch:SSE2")
		else()
			set_source_files_properties(${${MODULE_PREFIX}_SIMD_SOURCES} PROPERTIES COMPILE_FLAGS "-msse2" )
		endif()
	endif()

	list(APPEND ${MODULE_PREFIX}_SOURCES ${${MODULE_PREFIX}_SIMD_SOURCES})
	list(APPEND ${MODULE_PREFIX}_HEADERS ${${MODULE_PREFIX}_SIMD_HEADERS})
endif()

if(LIBYUV_FOUND)
	include_directories(${LIBYUV_INCLUDE_DIR})
endif()

add_library(${MODULE_NAME} STATIC ${${MODULE_PREFIX}_SOURCES} ${${MODULE_PREFIX}_HEADERS})

if(WITH_HALIDE)
	add_dependencies(${MODULE_NAME} halide-generate-all)
endif()

if(LIBYUV_FOUND)
	list(APPEND ${MODULE_PREFIX}_LIBS libyuv)
endif()

target_include_directories(${MODULE_NAME}
	INTERFACE $<INSTALL_INTERFACE:include>)

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})

if(XPP_INSTALL)
	install(TARGETS ${MODULE_NAME}
		EXPORT xpp-targets
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib)

	export(TARGETS ${MODULE_NAME}
		APPEND FILE ${PROJECT_BINARY_DIR}/xpp-targets.cmake)

	install(EXPORT xpp-targets
	        FILE xpp-targets.cmake
	        DESTINATION "${CMAKE_EXPORT_DIR}")
endif()

