SUBDIR( mocks )

SET( SRC ${SRC}

	${PWD}/Tests.cpp
	${PWD}/SmacGroundEdgeCost.cpp
	${PWD}/Async.cpp
	${PWD}/Backend.cpp
	${PWD}/RosterRecords.cpp
	${PWD}/SmacGameplayRandomState.cpp
	${PWD}/SmacScoutCombat.cpp
	${PWD}/SmacEmptyGroundMovement.cpp
	${PWD}/LiveRoster.cpp
	${PWD}/Buffer.cpp
	${PWD}/Mesh.cpp
	${PWD}/SmacBaseSquareYield.cpp
	${PWD}/SmacRulesTXTLoader.cpp
	${PWD}/UnitRulesTXTLoader.cpp
	${PWD}/UnitDesignResolver.cpp
	${PWD}/SmacHostRules.cpp
	${PWD}/SmacProductionRules.cpp
	${PWD}/SmacMovementRules.cpp
	${PWD}/SmacTechnologyRules.cpp
	${PWD}/SmacTechnologyState.cpp
	${PWD}/SmacSocialState.cpp
	${PWD}/SmacWorldSizeRules.cpp
	${PWD}/SmacPredefinedUnitCatalog.cpp
	${PWD}/SmacPredefinedUnitRules.cpp
	${PWD}/SmacFactionStartupRules.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/CvrContainerReader.cpp
	${PWD}/CvrSceneReader.cpp
	${PWD}/CvrGeometryReader.cpp
	${PWD}/SmacSimulationState.cpp
	${PWD}/SmacStartupDesignState.cpp
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
	${PWD}/SmacWorkerAllocation.cpp
	${PWD}/SmacWorkedSquareCollection.cpp
	${PWD}/SmacSpecialistAllocation.cpp
	${PWD}/SmacMineralCostFactor.cpp
	${PWD}/SmacConventionalStats.cpp

	PARENT_SCOPE )
