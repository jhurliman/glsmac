SUBDIR( backend )
SUBDIR( frontend )

SET( SRC ${SRC}

	${PWD}/UnitVisibility.cpp
	${PWD}/ScoutBattlePresentation.cpp
	${PWD}/FrontendRequest.cpp
	${PWD}/BackendRequest.cpp

	PARENT_SCOPE )
