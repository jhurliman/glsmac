# Match exact exit codes as well as diagnostics: timeout/crash is not rejection.
# Each invocation gets its own prefix and executes at most one native test or
# one script. In particular, none of these checks starts the full suite.
FUNCTION( CHECK_FILTER CASE EXPECTED_RESULT EXPECTED_OUTPUT EXPECTED_NATIVE_COUNT )
	EXECUTE_PROCESS(
		COMMAND "${GLSMAC_EXECUTABLE}" --quiet --prefix "${GLSMAC_TEST_PREFIX}-${CASE}" ${ARGN}
		WORKING_DIRECTORY "${GLSMAC_SOURCE_DIR}"
		RESULT_VARIABLE RESULT
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE ERROR_OUTPUT
		TIMEOUT 10
	)
	IF ( NOT "${RESULT}" STREQUAL "${EXPECTED_RESULT}" )
		MESSAGE( FATAL_ERROR "${CASE}: expected exit ${EXPECTED_RESULT}, got ${RESULT}\n${OUTPUT}\n${ERROR_OUTPUT}" )
	ENDIF ()
	STRING( FIND "${OUTPUT}${ERROR_OUTPUT}" "${EXPECTED_OUTPUT}" POSITION )
	IF ( POSITION EQUAL -1 )
		MESSAGE( FATAL_ERROR "${CASE}: missing diagnostic ${EXPECTED_OUTPUT}\n${OUTPUT}\n${ERROR_OUTPUT}" )
	ENDIF ()
	STRING( REGEX MATCHALL "  test [^\r\n]*" NATIVE_TESTS "${OUTPUT}" )
	LIST( LENGTH NATIVE_TESTS NATIVE_COUNT )
	IF ( NOT NATIVE_COUNT EQUAL EXPECTED_NATIVE_COUNT )
		MESSAGE( FATAL_ERROR "${CASE}: expected ${EXPECTED_NATIVE_COUNT} native tests, got ${NATIVE_COUNT}\n${OUTPUT}" )
	ENDIF ()
	IF ( NOT CASE STREQUAL "mesh_bridge" )
		STRING( FIND "${OUTPUT}" "  testing " SCRIPT_POSITION )
		IF ( NOT SCRIPT_POSITION EQUAL -1 )
			MESSAGE( FATAL_ERROR "${CASE}: unexpectedly executed a script\n${OUTPUT}" )
		ENDIF ()
	ENDIF ()
ENDFUNCTION ()

# The unanchored substring specifically checks regex_search semantics.
CHECK_FILTER( matching 0 "All tests passed." 1
	--gse-tests --gse-tests-native-filter "rectangle getters and render copies" )
CHECK_FILTER( invalid 1 "Invalid native test filter:" 0
	--gse-tests --gse-tests-native-filter "[" )
CHECK_FILTER( zero_match 1 "No native tests matched filter:" 0
	--gse-tests --gse-tests-native-filter "^no native test has this name$" )
CHECK_FILTER( missing_argument 1 "requires a value" 0
	--gse-tests --gse-tests-native-filter )
CHECK_FILTER( requires_test_mode 1 "only be used after --gse-tests" 0
	--gse-tests-native-filter "mesh" )
SET( SCRIPT "${GLSMAC_SOURCE_DIR}/tools/tests/mesh-memorydebug.gls.js" )
CHECK_FILTER( filter_then_script 1 "are mutually exclusive" 0
	--gse-tests --gse-tests-native-filter "mesh" --gse-tests-script "${SCRIPT}" )
CHECK_FILTER( script_then_filter 1 "are mutually exclusive" 0
	--gse-tests --gse-tests-script "${SCRIPT}" --gse-tests-native-filter "mesh" )
# Keep the script bridge exercised after moving UI/runtime fixtures to native
# execution. Only the three pure mesh checks remain callable from scripts.
CHECK_FILTER( mesh_bridge 0 "All tests passed." 0
	--gse-tests --gse-tests-script "${SCRIPT}" )
