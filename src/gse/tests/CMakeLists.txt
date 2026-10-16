SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/Backend.cpp
	${PWD}/Buffer.cpp
	${PWD}/Mesh.cpp
	${PWD}/SmacBaseSquareYield.cpp
	${PWD}/SmacRulesTXTLoader.cpp
	${PWD}/SmacHostRules.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/GSE.cpp
	${PWD}/Parser.cpp
	${PWD}/Runner.cpp
	${PWD}/Scripts.cpp

	PARENT_SCOPE )
