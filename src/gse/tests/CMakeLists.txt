SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/Async.cpp
	${PWD}/Backend.cpp
	${PWD}/RosterRecords.cpp
	${PWD}/SmacGameplayRandomState.cpp
	${PWD}/LiveRoster.cpp
	${PWD}/Buffer.cpp
	${PWD}/Mesh.cpp
	${PWD}/SmacBaseSquareYield.cpp
	${PWD}/SmacRulesTXTLoader.cpp
	${PWD}/UnitRulesTXTLoader.cpp
	${PWD}/UnitDesignResolver.cpp
	${PWD}/SmacHostRules.cpp
	${PWD}/SmacProductionRules.cpp
	${PWD}/SmacTechnologyRules.cpp
	${PWD}/SmacPredefinedUnitCatalog.cpp
	${PWD}/SmacFactionStartupRules.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/CvrContainerReader.cpp
	${PWD}/CvrSceneReader.cpp
	${PWD}/CvrGeometryReader.cpp
	${PWD}/CvrFrameAssembler.cpp
	${PWD}/CvrIndexedRasterizer.cpp
	${PWD}/CvrBasisConverter.cpp
	${PWD}/CvrTransformCalculator.cpp
	${PWD}/CvrScoutRenderer.cpp
	${PWD}/GSE.cpp
	${PWD}/Parser.cpp
	${PWD}/Runner.cpp
	${PWD}/Scripts.cpp
	${PWD}/SmacProductionSwitch.cpp
	${PWD}/SmacMineralCostFactor.cpp

	PARENT_SCOPE )
