// Keep an included closure and its child context alive until GSE teardown.
const create_counter = () => {
	let value = 0;
	return () => { value++; return value; };
};

return {next: create_counter()};
