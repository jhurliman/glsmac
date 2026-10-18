return (m) => {
 m.glsmac.on('intro', (e) => {
  const backdrop = m.root.surface({ left: 0, right: 0, top: 0, bottom: 0, zindex: 0.0, background: '#000000' });
  const movie = m.root.surface({ align: 'center', zindex: 0.9, width: 1, height: 1 });
  const paths = [ 'movies/logo.wve', e.opening_movie ];
  let index = 0;
  let closed = false;
  const finish = () => {
   if (closed) { return; }
   closed = true;
   movie.stopmovie(); movie.remove(); backdrop.remove(); m.glsmac.mainmenu();
  };
  const next = () => {
   movie.stopmovie();
   let playing = false;
   while (index < 2 && !playing) { playing = movie.playmovie(paths[index]); index++; }
   if (!playing) { finish(); }
  };
  movie.listen(m.root, 'keydown', (key) => {
   if (key.code == 'ESCAPE' || key.code == 'SPACE' || key.code == 'ENTER') { finish(); return true; }
   return false;
  });
  movie.listen(m.root, 'mousedown', (mouse) => { finish(); return true; });
  movie.timer(10, () => {
   if (closed) { return false; }
   if (!movie.tickmovie()) { next(); }
   return !closed;
  });
  next();
 });
};
