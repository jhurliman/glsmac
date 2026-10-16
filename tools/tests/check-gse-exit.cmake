# Check the exact result and diagnostic: a crash or missing executable must
# never count as a correctly reported test failure.
IF ( TEST_CASE STREQUAL "success" )
	SET( SCRIPT "${GLSMAC_SOURCE_DIR}/GLSMAC_data/tests/empty.gls.js" )
	SET( EXPECTED_RESULT 0 )
	SET( EXPECTED_OUTPUT "All tests passed." )
ELSEIF ( TEST_CASE STREQUAL "failure" )
	SET( SCRIPT "${GLSMAC_SOURCE_DIR}/tools/tests/intentional-failure.gls.js" )
	SET( EXPECTED_RESULT 1 )
	SET( EXPECTED_OUTPUT "Assertion failed: intentional exit-status regression failure" )
ELSEIF ( TEST_CASE STREQUAL "missing" )
	SET( SCRIPT "${GLSMAC_SOURCE_DIR}/tools/tests/does-not-exist.gls.js" )
	SET( EXPECTED_RESULT 1 )
	SET( EXPECTED_OUTPUT "does not exist or is not a file" )
ELSE ()
	MESSAGE( FATAL_ERROR "Unknown test case: ${TEST_CASE}" )
ENDIF ()

EXECUTE_PROCESS(
	COMMAND "${GLSMAC_EXECUTABLE}" --gse-tests --gse-tests-script "${SCRIPT}" --quiet --prefix "${GLSMAC_TEST_PREFIX}"
	WORKING_DIRECTORY "${GLSMAC_SOURCE_DIR}"
	RESULT_VARIABLE RESULT
	OUTPUT_VARIABLE OUTPUT
	ERROR_VARIABLE ERROR_OUTPUT
	TIMEOUT 20
)
IF ( NOT "${RESULT}" STREQUAL "${EXPECTED_RESULT}" )
	MESSAGE( FATAL_ERROR "Expected exit ${EXPECTED_RESULT}, got ${RESULT}\n${OUTPUT}\n${ERROR_OUTPUT}" )
ENDIF ()
STRING( FIND "${OUTPUT}" "${EXPECTED_OUTPUT}" OUTPUT_POSITION )
IF ( OUTPUT_POSITION EQUAL -1 )
	MESSAGE( FATAL_ERROR "Missing expected diagnostic: ${EXPECTED_OUTPUT}\n${OUTPUT}\n${ERROR_OUTPUT}" )
ENDIF ()
