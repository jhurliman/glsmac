SUBDIR( generator )
SUBDIR( module )
SUBDIR( tile )

SET( SRC ${SRC}
    ${PWD}/SmacRetainedTerrain.cpp
    ${PWD}/SmacStartupTerritory.cpp
    ${PWD}/SmacRawSiteQuality.cpp
    ${PWD}/SmacGeneratedStartingSite.cpp

	${PWD}/SmacClimate.cpp
	${PWD}/SmacGenerationBootstrap.cpp
	${PWD}/SmacContinentGeneration.cpp
	${PWD}/SmacGenerationNoise.cpp
	${PWD}/SmacGenerationRegions.cpp
	${PWD}/SmacPretemperatureGeneration.cpp
	${PWD}/SmacInitialClimateGeneration.cpp
	${PWD}/SmacFinalGeneration.cpp
	${PWD}/SmacLandmarkGeneration.cpp
	${PWD}/SmacCompleteTerrainGeneration.cpp
	${PWD}/SmacTerrainAuthority.cpp
	${PWD}/SmacGlobalSpread.cpp
	${PWD}/MapGlobalSpread.cpp
	${PWD}/SmacGlobalWildlife.cpp
	${PWD}/SmacTerrainLighting.cpp
	${PWD}/SmacOrdinaryLandExploration.cpp
	${PWD}/Consts.cpp
	${PWD}/Map.cpp
	${PWD}/SmacOccupancyCache.cpp
	${PWD}/SmacPlanetMap.cpp
	${PWD}/MapPlanet.cpp
    ${PWD}/MapNativeTerrain.cpp
	${PWD}/MapTerrainUpdate.cpp
	${PWD}/TerrainRenderUpdate.cpp
	${PWD}/GlsmacResourceMapPolicy.cpp
	${PWD}/SmacWorkerMapFrame.cpp
	${PWD}/SmacWorkerKnowledge.cpp
	${PWD}/SmacMinimap.cpp
	${PWD}/SmacTerritoryRules.cpp
	${PWD}/SmacSiteQuality.cpp
	${PWD}/SmacSiteRatingCache.cpp
	${PWD}/SmacRestartSearch.cpp
	${PWD}/SmacStartingSite.cpp
	${PWD}/SmacStartingBalance.cpp
	${PWD}/SmacRestartPreprocessing.cpp
	${PWD}/GlsmacTerritoryTopology.cpp
	${PWD}/SmacTerritorySummary.cpp
	${PWD}/MapTerritorySummary.cpp
	${PWD}/MapTerritory.cpp
	${PWD}/MapSiteRatings.cpp
	${PWD}/MapState.cpp

	PARENT_SCOPE )
