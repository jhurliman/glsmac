const intro = #include('../default/ui/parts/intro');
const exercise = (missing, skip) => {
 let handler = null; let timer = null; let menus = 0; let removed = 0; let played = []; let keys = null; let mouse = null;
 const movie = {
  stopmovie: () => {},
  remove: () => { removed++; },
  playmovie: (path) => { played :+ path; return !missing; },
  tickmovie: () => { return false; },
  listen: (root, event, callback) => { if (event == 'keydown') { keys = callback; } else { mouse = callback; } },
  timer: (ms, callback) => { test.assert(ms == 10); timer = callback; },
 };
 let created = 0;
 const root = {surface: (props) => {
  created++; if (created == 1) { return {remove: () => { removed++; }}; } return movie;
 }};
 const glsmac = {on: (event, callback) => { test.assert(event == 'intro'); handler = callback; }, mainmenu: () => { menus++; }};
 intro({root: root, glsmac: glsmac});
 handler({opening_movie: 'movies/opening.wve'});
 if (skip) {
  test.assert(keys({code: 'ESCAPE'}));
  test.assert(mouse({}));
 } else if (!missing) {
  test.assert(timer());
  test.assert(!timer());
 }
 test.assert(menus == 1 && removed == 2 && !timer());
 test.assert(played[0] == 'movies/logo.wve');
 if (!skip) { test.assert(#sizeof(played) == 2 && played[1] == 'movies/opening.wve'); }
};
exercise(false, false);
exercise(false, true);
exercise(true, false);
