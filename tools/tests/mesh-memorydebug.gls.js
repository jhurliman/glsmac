// Pure mesh assertions are safe inside script accumulation. The UI ownership
// fixtures run separately through --gse-tests-native-filter in CTest.
const error = test.check_mesh_coordinate_outputs();
test.assert(error == '', error);
