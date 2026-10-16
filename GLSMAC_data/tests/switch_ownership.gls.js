// Exercise retained switch scopes through the real parser and interpreter.
let untouched = 7;
switch (1) {}
test.assert(untouched == 7);

const choose = (input) => {
	let result = 0;
	switch (input) {
		case 0: {
			result = 1;
			break;
		}
		case 1: {
			result = 10;
			// GSE falls through into the following case.
		}
		case 2: {
			result += 2;
			break;
		}
		case 3: {
			switch (1) {
				case 1: {
					result = 30;
					break;
				}
				default: {
					result = 31;
				}
			}
			result += 3;
			break;
		}
		default: {
			result = -1;
		}
	}
	return result;
};

test.assert(choose(0) == 1);
test.assert(choose(1) == 12);
test.assert(choose(2) == 2);
test.assert(choose(3) == 33);
test.assert(choose(9) == -1);
