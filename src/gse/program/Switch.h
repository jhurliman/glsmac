#pragma once

#include <vector>
#include <memory>

#include "Conditional.h"

namespace gse {
namespace program {

class SimpleCondition;
class Case;
class Scope;

class Switch : public Conditional {
public:

	Switch( const si_t& si, const SimpleCondition* condition, std::unique_ptr< const Scope > body );
	~Switch();

	const SimpleCondition* condition;
	const std::vector< Case* > cases; // borrowed from m_body

	const std::string Dump( const size_t depth = 0 ) const override;

private:
	const std::unique_ptr< const Scope > m_body;
};

}
}

