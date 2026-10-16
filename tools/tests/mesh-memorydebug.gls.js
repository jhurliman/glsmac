// CTest enables allocation tracking from startup, then runs the same native
// assertions as the full suite without initializing the renderer or assets.
const error = test.check_mesh_coordinate_outputs();
test.assert(error == '', error);
