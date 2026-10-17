// The dedicated CTest enables allocation tracking before application startup.
// This helper invokes the production UUID exporter, without account-file state.
for (let i = 0; i < 32; i++) {
	const id = test.generate_uuid();
	test.assert(#sizeof(#split(id, '-')) == 5, 'UUID export returned an invalid text format');
}
