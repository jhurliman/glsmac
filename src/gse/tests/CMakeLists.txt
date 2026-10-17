SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/Backend.cpp
	${PWD}/Buffer.cpp
	${PWD}/SmacBaseSquareYield.cpp
	${PWD}/SmacRulesTXTLoader.cpp
	${PWD}/SmacHostRules.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/SmacProductionRules.cpp
	${PWD}/SmacTechnologyRules.cpp
	${PWD}/SmacWorldSizeRules.cpp
	${PWD}/CvrContainerReader.cpp
	${PWD}/CvrSceneReader.cpp
	${PWD}/CvrGeometryReader.cpp
	${PWD}/GSE.cpp
	${PWD}/Parser.cpp
	${PWD}/Runner.cpp
	${PWD}/Scripts.cpp
	${PWD}/SmacMineralCostFactor.cpp

	PARENT_SCOPE )
