SUBDIR( map )
SUBDIR( resource )
SUBDIR( rules )
SUBDIR( settings )
SUBDIR( slot )
SUBDIR( connection )
SUBDIR( faction )
SUBDIR( animation )
SUBDIR( unit )
SUBDIR( base )
SUBDIR( event )
SUBDIR( turn )

SET( SRC ${SRC}

	${PWD}/Random.cpp
	${PWD}/SmacGameplayRandomState.cpp
	${PWD}/SmacMovementHistory.cpp
	${PWD}/Bindings.cpp
	${PWD}/State.cpp
	${PWD}/WorldSnapshotRules.cpp
	${PWD}/WorldSnapshotWorkerKnowledge.cpp
	${PWD}/WorldSnapshotTerritory.cpp
	${PWD}/WorldSnapshotRandom.cpp
	${PWD}/WorldSnapshotTurn.cpp
	${PWD}/Game.cpp
	${PWD}/GameSimulation.cpp
	${PWD}/GameTileResources.cpp
	${PWD}/GameWorkerKnowledge.cpp
	${PWD}/GameTerritory.cpp
	${PWD}/Account.cpp
	${PWD}/Player.cpp
	${PWD}/RosterRecords.cpp
	${PWD}/StateRoster.cpp
	${PWD}/MapObject.cpp
	${PWD}/ResourceRelated.cpp

	PARENT_SCOPE )
