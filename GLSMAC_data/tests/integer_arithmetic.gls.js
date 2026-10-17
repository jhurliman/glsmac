const maximum = 9223372036854775807;
const minimum = 0 - maximum - 1;

const expect_math_error = (operation) => {
	let caught = false;
	let reason = '';
	try { operation(); }
	catch { GSEMathError: (e) => { caught = true; reason = e.reason; } }
	test.assert(caught, 'expected a catchable integer arithmetic error');
	test.assert(reason == 'Integer overflow' || reason == 'Division by zero');
	return reason;
};

// Representable endpoints, signs, truncation toward zero and remainder signs.
test.assert(maximum + 0 == maximum && minimum - 0 == minimum);
test.assert((maximum - 1) + 1 == maximum && (minimum + 1) - 1 == minimum);
test.assert(maximum + minimum == -1 && minimum - minimum == 0);
test.assert(minimum * 0 == 0 && minimum * 1 == minimum);
test.assert(maximum * -1 == 0 - maximum && (minimum + 1) * -1 == maximum);
test.assert(-2 * -3 == 6 && -2 * 3 == -6 && 2 * -3 == -6);
test.assert(3037000499 * 3037000499 == 9223372030926249001);
test.assert(minimum / 1 == minimum && maximum / -1 == 0 - maximum);
test.assert(-7 / 3 == -2 && 7 / -3 == -2 && -7 / -3 == 2);
test.assert(-7 % 3 == -1 && 7 % -3 == 1 && minimum % 1 == 0);
test.assert(#abs(0) == 0 && #abs(0 - maximum) == maximum && #abs(maximum) == maximum);

const operations = [
	() => { maximum + 1; }, () => { minimum + -1; },
	() => { minimum - 1; }, () => { maximum - -1; }, () => { 0 - minimum; },
	() => { maximum * 2; }, () => { minimum * 2; },
	() => { minimum * -1; }, () => { -1 * minimum; },
	() => { 3037000500 * 3037000500; },
	() => { minimum / -1; }, () => { minimum % -1; },
	() => { 1 / 0; }, () => { 0 / 0; }, () => { 1 % 0; },
	() => { #abs(minimum); }
];
for (operation of operations) { expect_math_error(operation); }

// Failed compound operations must not write an invalid result to the target.
// GSE currently spells compound remainder %/ (see parser/JS.h).
let target = maximum;
expect_math_error(() => { target += 1; });
test.assert(target == maximum);
target = minimum;
expect_math_error(() => { target -= 1; });
test.assert(target == minimum);
expect_math_error(() => { target *= -1; });
test.assert(target == minimum);
expect_math_error(() => { target /= -1; });
test.assert(target == minimum);
expect_math_error(() => { target %/ -1; });
test.assert(target == minimum);
test.assert(expect_math_error(() => { target /= 0; }) == 'Division by zero');
test.assert(target == minimum);
test.assert(expect_math_error(() => { target %/ 0; }) == 'Division by zero');
test.assert(target == minimum);

// Prefix/postfix failures preserve both the target and an assignment's old value.
let captured = 123;
target = maximum;
expect_math_error(() => { captured = target++; });
test.assert(target == maximum && captured == 123);
expect_math_error(() => { captured = ++target; });
test.assert(target == maximum && captured == 123);
target = minimum;
expect_math_error(() => { captured = target--; });
test.assert(target == minimum && captured == 123);
expect_math_error(() => { captured = --target; });
test.assert(target == minimum && captured == 123);

// Successful mutations preserve the old/new return-value distinction.
target = maximum - 1;
captured = target++;
test.assert(captured == maximum - 1 && target == maximum);
captured = --target;
test.assert(captured == maximum - 1 && target == maximum - 1);
target = minimum + 1;
captured = target--;
test.assert(captured == minimum + 1 && target == minimum);
captured = ++target;
test.assert(captured == minimum + 1 && target == minimum + 1);
target = -7;
target += 1; target -= 1; target *= 3; target /= 2; target %/ 3;
test.assert(target == -1);

// The RHS still executes once; overflow is not a transaction on its side effects.
let evaluations = 0;
const rhs = () => { evaluations++; return 1; };
target = maximum;
expect_math_error(() => { target += rhs(); });
test.assert(evaluations == 1 && target == maximum);
test.assert((maximum - 1) + rhs() == maximum && evaluations == 2);
