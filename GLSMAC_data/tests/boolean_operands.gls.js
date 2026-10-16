// Boolean operators accept several AST operand types, not just expressions.
test.assert(!false);
test.assert((!true) == false);

const enabled = true;
const disabled = false;
test.assert(!disabled);
test.assert((!enabled) == false);
test.assert(enabled && !disabled);
test.assert(disabled || enabled);

const as_bool = (value) => { return value; };
test.assert(!as_bool(false));
test.assert(as_bool(true) && (2 > 1));
test.assert(as_bool(false) || (1 < 2));

let calls = 0;
const visit = (value) => {
	calls++;
	return value;
};
test.assert((false && visit(true)) == false);
test.assert(true || visit(false));
test.assert(calls == 0, 'short-circuited operands must not run');
test.assert(true && visit(true));
test.assert(false || visit(true));
test.assert(calls == 2, 'required operands must run exactly once');

// GSE requires actual Bool values; it does not coerce other value types.
const expect_bool_error = (f) => {
	let caught = false;
	try {
		f();
	} catch {
		GSETypeError: (error) => { caught = true; }
	}
	test.assert(caught, 'non-boolean operand must raise GSETypeError');
};
expect_bool_error(() => { !0; });
expect_bool_error(() => { !''; });
expect_bool_error(() => { !null; });
expect_bool_error(() => { ![]; });
expect_bool_error(() => { true && 1; });
expect_bool_error(() => { false || 'yes'; });
