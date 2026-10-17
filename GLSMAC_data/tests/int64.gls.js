// GSE Int is signed 64-bit on every target, including Windows (32-bit long).
const maximum = 9223372036854775807;
const minimum = 0 - maximum - 1;
test.assert(#to_string(2147483648) == '2147483648');
test.assert(#to_string(maximum) == '9223372036854775807');
test.assert(#to_string(minimum) == '-9223372036854775808');
test.assert(#to_int('2147483648') == 2147483648);
test.assert(#to_int('9223372036854775807') == maximum);
test.assert(#to_int('-9223372036854775808') == minimum);
for (text of ['9223372036854775808', '-9223372036854775809']) {
	let rejected = false;
	try {
		#to_int(text);
	} catch {
		GSEConversionError: (e) => { rejected = true; }
	}
	test.assert(rejected, 'out-of-range integer conversion was accepted');
}
