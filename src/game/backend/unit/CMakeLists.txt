SET( SRC ${SRC}
    ${PWD}/SmacNativeAI.cpp
	${PWD}/SmacGroundEdgeCost.cpp
	${PWD}/SmacScoutKeyboardAdmission.cpp
	${PWD}/SmacPredefinedUnitCatalog.cpp
	${PWD}/SmacStartupDesignState.cpp
	${PWD}/SmacRestartUnitPackage.cpp
	${PWD}/SmacEmptyGroundMovement.cpp

	${PWD}/UnitManager.cpp
	${PWD}/UnitTransfer.cpp
	${PWD}/ConventionalMovement.cpp
	${PWD}/ConventionalCombat.cpp
	${PWD}/ConventionalWork.cpp
	${PWD}/ConventionalOrders.cpp
	${PWD}/Def.cpp
	${PWD}/StaticDef.cpp
	${PWD}/Render.cpp
	${PWD}/SpriteRender.cpp
	${PWD}/CvrScoutRender.cpp
	${PWD}/Unit.cpp
	${PWD}/SmacNativeUnitRecord.cpp
	${PWD}/NativeUnit.cpp
	${PWD}/ConventionalUnitState.cpp
	${PWD}/SmacConventionalStats.cpp
	${PWD}/Morale.cpp
	${PWD}/MoraleSet.cpp
	${PWD}/SmacScoutCombat.cpp

	PARENT_SCOPE )
