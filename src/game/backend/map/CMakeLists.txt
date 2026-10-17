SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}

	${PWD}/SmacOrdinaryLandExploration.cpp
	${PWD}/Consts.cpp
	${PWD}/Map.cpp
	${PWD}/GlsmacResourceMapPolicy.cpp
	${PWD}/SmacWorkerMapFrame.cpp
	${PWD}/SmacWorkerKnowledge.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
