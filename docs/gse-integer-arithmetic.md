# GSE integer arithmetic

GSE integers remain signed 64-bit values. Addition, subtraction, multiplication,
division, remainder, compound assignments, prefix/postfix increment/decrement,
and integer `#abs` check their inputs before performing an operation that could
overflow. Unrepresentable results raise catchable `GSEMathError` with reason
`Integer overflow`. Integer division and remainder by zero retain the reason
`Division by zero`. Both minimum-int divided by minus one and its remainder
raise overflow; neither evaluates C++'s undefined quotient operation.

Representable results retain truncation toward zero for division and the
dividend's sign for a nonzero remainder. Operand evaluation order and count,
strict type matching, and prefix/postfix return values are unchanged. A failed
arithmetic mutation leaves its target unchanged. Side effects from evaluating
the operands still happen; arithmetic errors do not roll them back.

The implementation uses portable signed bounds checks before arithmetic. It
does not substitute wrapping, saturation or floating-point results. Float
operations, `#pow`, conversions and parsing are outside this change. In
particular, the existing compound-remainder spelling is `%/`, and unary minus
on a variable remains unsupported; `0 - value` uses checked subtraction.

The regular script suite exercises endpoints, every mutation path, captured
prefix/postfix results, catch-and-continue behavior, and once-only RHS effects.
No additional CTest process or original game assets are required.
