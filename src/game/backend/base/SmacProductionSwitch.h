#pragma once

#include <set>

#include "ProductionState.h"

namespace game::backend::base {

// Explicit SMAC inputs. Facility membership and semantic item kinds are supplied
// by the caller; this calculator performs no catalog, eligibility or world lookup.
struct SmacProductionSwitchContext {
	int32_t strictness;
	int32_t loss_percent;
	int32_t exemption;
	bool human;
	int32_t stored_difficulty;
	bool skunkworks;
	std::set< std::string > present_local_facilities;
};

enum class SmacPenaltyChoice { Accept, Cancel };

struct SmacAutomaticSwitchResult {
	ProductionState state;
	bool refresh_requested;
};

struct SmacManualSwitchResult {
	ProductionState state;
	bool penalty_prompt_required;
	bool penalty_cancelled;
};

// Owned results only; input and baseline fields never change. Requires configured
// state, signed32 balances, strictness 0..3, loss 0..100 and representable evaluated
// signed32 intermediates. Invalid inputs/arithmetic throw std::invalid_argument.
// Counter updates, chooser admission, queues and live mutation are not implemented.
class SmacProductionSwitch {
public:
	// nullopt is a distinct keep-current request, not Stockpile or unconfigured.
	static SmacAutomaticSwitchResult Automatic( const ProductionState& state, const std::optional< ProductionItem >& target, const SmacProductionSwitchContext& context );
	// Choice is consumed only if the admitted primary replacement needs a prompt.
	static SmacManualSwitchResult Manual( const ProductionState& state, const ProductionItem& target, SmacPenaltyChoice choice, const SmacProductionSwitchContext& context );
	// A baseline-derived inquiry, not the amount removed by the last mutation.
	static int32_t BaselineLoss( const ProductionState& state, const SmacProductionSwitchContext& context );
};

}
