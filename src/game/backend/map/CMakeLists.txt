SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}

	${PWD}/Consts.cpp
	${PWD}/SmacTerritoryRules.cpp
	${PWD}/Map.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
