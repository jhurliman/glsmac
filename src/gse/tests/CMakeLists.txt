SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/Async.cpp
	${PWD}/Backend.cpp
	${PWD}/Buffer.cpp
	${PWD}/Value.cpp
	${PWD}/Faction.cpp
	${PWD}/Mesh.cpp
	${PWD}/SmacBaseSquareYield.cpp
	${PWD}/SmacTileMinerals.cpp
	${PWD}/SmacTileEnergy.cpp
	${PWD}/SmacTileNutrients.cpp
	${PWD}/SmacResourceBonus.cpp
	${PWD}/SmacRulesTXTLoader.cpp
	${PWD}/UnitRulesTXTLoader.cpp
	${PWD}/UnitDesignResolver.cpp
	${PWD}/CvrContainerReader.cpp
	${PWD}/CvrSceneReader.cpp
	${PWD}/CvrGeometryReader.cpp
	${PWD}/SmacHostRules.cpp
	${PWD}/SmacProductionRules.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/CvrFrameAssembler.cpp
	${PWD}/CvrIndexedRasterizer.cpp
	${PWD}/CvrBasisConverter.cpp
	${PWD}/CvrTransformCalculator.cpp
	${PWD}/GSE.cpp
	${PWD}/Parser.cpp
	${PWD}/Runner.cpp
	${PWD}/Scripts.cpp
	${PWD}/SmacMineralCostFactor.cpp
	${PWD}/SmacProductionSwitch.cpp

	PARENT_SCOPE )
