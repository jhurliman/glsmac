# Real argument parsing, with the empty headless GSE script as the successful
# launch target. A crash or missing executable cannot count as rejection.
FUNCTION( CHECK_RULESET RESULT_EXPECTED TEXT_EXPECTED )
	EXECUTE_PROCESS(
		COMMAND "${GLSMAC_EXECUTABLE}" ${ARGN} --gse-tests --quiet
			--gse-tests-script "${GLSMAC_SOURCE_DIR}/GLSMAC_data/tests/empty.gls.js"
			--prefix "${GLSMAC_TEST_PREFIX}"
		WORKING_DIRECTORY "${GLSMAC_SOURCE_DIR}"
		RESULT_VARIABLE RESULT
		OUTPUT_VARIABLE OUTPUT
		ERROR_VARIABLE ERROR_OUTPUT
		TIMEOUT 20
	)
	IF ( NOT "${RESULT}" STREQUAL "${RESULT_EXPECTED}" )
		MESSAGE( FATAL_ERROR "Args ${ARGN}: expected exit ${RESULT_EXPECTED}, got ${RESULT}\n${OUTPUT}\n${ERROR_OUTPUT}" )
	ENDIF ()
	STRING( FIND "${OUTPUT}${ERROR_OUTPUT}" "${TEXT_EXPECTED}" POSITION )
	IF ( POSITION EQUAL -1 )
		MESSAGE( FATAL_ERROR "Args ${ARGN}: missing diagnostic ${TEXT_EXPECTED}\n${OUTPUT}\n${ERROR_OUTPUT}" )
	ENDIF ()
ENDFUNCTION ()

CHECK_RULESET( 1 "require --ruleset" --quickstart )
CHECK_RULESET( 1 "require --ruleset" --host )
CHECK_RULESET( 1 "Invalid --ruleset value" --ruleset gog )
CHECK_RULESET( 1 "uses the host's ruleset" --join localhost --ruleset smac )
CHECK_RULESET( 1 "SMAC quickstart requires" --quickstart --ruleset smac --smactype gog )
CHECK_RULESET( 1 "requires a preset" --quickstart --ruleset smac --quickstart-world-size 5 )
CHECK_RULESET( 1 "requires a preset" --quickstart --ruleset smac --quickstart-world-size 1junk )
CHECK_RULESET( 1 "requires SMAC" --quickstart --ruleset smax --quickstart-world-size 1 )
CHECK_RULESET( 1 "cannot be combined" --quickstart --ruleset smac --quickstart-world-size 1 --quickstart-mapsize 48x48 )
CHECK_RULESET( 0 "All tests passed." --quickstart --ruleset smac --smactype gog --quickstart-world-size 0 )
CHECK_RULESET( 0 "All tests passed." --quickstart --ruleset smac --quickstart-world-size 4 )
CHECK_RULESET( 0 "All tests passed." --quickstart --ruleset smac --quickstart-mapsize 48x48 )
CHECK_RULESET( 0 "All tests passed." --quickstart --ruleset smax )
CHECK_RULESET( 0 "All tests passed." --host --ruleset smax --smactype legacy )
CHECK_RULESET( 0 "All tests passed." --join localhost )
