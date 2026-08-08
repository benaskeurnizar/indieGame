# materials/

Optional. If `materials/materials.txt` exists, `mapTextures()` in `src/render_bsp.cpp` uses it
to swap a BSP texture name for a real image file (loaded with `stb_image`) instead of the
built-in Quake palette colors.

This folder is currently empty in this repo — without `materials.txt`, textures just render
using the palette baked into the `.bsp`/`palette.lmp`, which still works fine, just flatter
and lower-res than a real texture would look.

Format, read directly from `mapTextures()` in `src/render_bsp.cpp`: one entry per line, a
comma-separated list of BSP texture names, then a single space, then the path to the image
file to use for all of them:

```
afloor3_t,city2_2,city2_3 materials/marble_texture1.png
city5_3 materials/concrete_texture1.png
```

No spaces allowed inside the texture-name list (it's split on the first space in the line).
