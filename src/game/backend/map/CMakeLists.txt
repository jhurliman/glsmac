SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}

	${PWD}/SmacClimate.cpp
	${PWD}/SmacOrdinaryLandExploration.cpp
	${PWD}/Consts.cpp
	${PWD}/Map.cpp
	${PWD}/SmacPlanetMap.cpp
	${PWD}/MapPlanet.cpp
	${PWD}/MapTerrainUpdate.cpp
	${PWD}/TerrainRenderUpdate.cpp
	${PWD}/GlsmacResourceMapPolicy.cpp
	${PWD}/SmacWorkerMapFrame.cpp
	${PWD}/SmacWorkerKnowledge.cpp
	${PWD}/SmacTerritoryRules.cpp
	${PWD}/SmacSiteQuality.cpp
	${PWD}/SmacSiteRatingCache.cpp
	${PWD}/SmacRestartSearch.cpp
	${PWD}/SmacRestartPreprocessing.cpp
	${PWD}/GlsmacTerritoryTopology.cpp
	${PWD}/SmacTerritorySummary.cpp
	${PWD}/MapTerritorySummary.cpp
	${PWD}/MapTerritory.cpp
	${PWD}/MapSiteRatings.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
