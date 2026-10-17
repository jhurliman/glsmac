#include "Switch.h"

#include "SimpleCondition.h"
#include "Case.h"
#include "Scope.h"
#include "common/Assert.h"

namespace gse {
namespace program {
namespace {

std::vector< Case* > GetCases( const Scope& body ) {
	std::vector< Case* > cases;
	cases.reserve( body.body.size() );
	for ( const auto* control : body.body ) {
		ASSERT( control->control_type == Control::CT_CONDITIONAL, "control type not conditional" );
		ASSERT( static_cast< const Conditional* >( control )->conditional_type == Conditional::CT_CASE, "conditional type not case" );
		cases.push_back( (Case*)control );
	}
	return cases;
}

}

Switch::Switch( const si_t& si, const SimpleCondition* condition, std::unique_ptr< const Scope > body )
	: Conditional( si, CT_SWITCH )
	, condition( condition )
	, cases( GetCases( *body ) )
	, m_body( std::move( body ) ) {}

Switch::~Switch() {
	delete condition;
}

const std::string Switch::Dump( const size_t depth ) const {
	std::string result = Formatted( "Switch" + m_si.ToString() + "(", depth ) +
		Formatted( "Condition(", depth + 1 ) +
		condition->Dump( depth + 2 ) +
		Formatted( ")", depth + 1 ) +
		Formatted( "Body(", depth + 1 );
	for ( const auto& c : cases ) {
		result += c->Dump( depth + 3 );
	}
	result += Formatted( ")", depth + 1 ) +
		Formatted( ")", depth );
	return result;
}

}
}
