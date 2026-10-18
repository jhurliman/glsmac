SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}

	${PWD}/SmacOrdinaryLandExploration.cpp
	${PWD}/Consts.cpp
	${PWD}/Map.cpp
	${PWD}/MapTerrainUpdate.cpp
	${PWD}/TerrainRenderUpdate.cpp
	${PWD}/GlsmacResourceMapPolicy.cpp
	${PWD}/SmacWorkerMapFrame.cpp
	${PWD}/SmacWorkerKnowledge.cpp
	${PWD}/SmacTerritoryRules.cpp
	${PWD}/SmacSiteQuality.cpp
	${PWD}/SmacSiteRatingCache.cpp
	${PWD}/GlsmacTerritoryTopology.cpp
	${PWD}/MapTerritory.cpp
	${PWD}/MapSiteRatings.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
