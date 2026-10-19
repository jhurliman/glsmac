SUBDIR( actor )
SUBDIR( faction )
SUBDIR( tile )
SUBDIR( sprite )
SUBDIR( text )
SUBDIR( unit )
SUBDIR( base )
SUBDIR( resource )
SUBDIR( widget )

SET( SRC ${SRC}

	${PWD}/Game.cpp
	${PWD}/GameScoutBattle.cpp
	${PWD}/SmacTerrainRaster.cpp
	${PWD}/SmacLandMaterial.cpp
    ${PWD}/SmacWaterCoast.cpp
	${PWD}/NativeTerrainRenderer.cpp
    ${PWD}/GameNativeTerrain.cpp
	${PWD}/Slot.cpp
	${PWD}/TileObject.cpp
	${PWD}/AnimationDef.cpp
	${PWD}/Animation.cpp

	PARENT_SCOPE )
