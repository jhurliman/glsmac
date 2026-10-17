// Opt-in, asset-free Debug/Release checks against the production assembler.
#include "gse/tests/fixtures/CvrFrameArithmetic.h"

#include <iostream>

int main() {
	try {
		const auto error = gse::tests::cvr_frame_arithmetic::Check();
		if ( !error.empty() ) { std::cerr << error << '\n'; return 1; }
		std::cout << "CVR arithmetic environment checks passed\n";
		return 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
