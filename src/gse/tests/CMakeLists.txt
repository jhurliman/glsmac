SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/Backend.cpp
	${PWD}/Buffer.cpp
	${PWD}/CvrContainerReader.cpp
	${PWD}/CvrSceneReader.cpp
	${PWD}/CvrGeometryReader.cpp
	${PWD}/GSE.cpp
	${PWD}/Parser.cpp
	${PWD}/Runner.cpp
	${PWD}/Scripts.cpp
	${PWD}/SmacProductionSwitch.cpp

	PARENT_SCOPE )
