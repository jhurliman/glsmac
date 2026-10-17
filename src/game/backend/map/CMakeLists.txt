SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}

	${PWD}/Consts.cpp
	${PWD}/Map.cpp
	${PWD}/GlsmacResourceMapPolicy.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
