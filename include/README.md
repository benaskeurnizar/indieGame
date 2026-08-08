# include/

Not checked into git — holds third-party headers that `src/build.bat` expects via
`/I "..\..\include"`. Populate it yourself:

```
include/
├── glad/
│   └── glad.h              # generate at https://glad.dav1d.de (OpenGL loader)
├── cglm.h                   # from https://github.com/recp/cglm
├── mat4.h
├── vec3.h
├── vec4.h
└── stb/
    └── stb_image.h          # from https://github.com/nothings/stb (public domain)
```

**glad**: generate a loader for the OpenGL version/profile you're targeting and drop the
generated `glad.h` (and `KHR/khrplatform.h`) here.

**cglm**: only `cglm.h`, `mat4.h`, `vec3.h`, `vec4.h` are pulled in directly (not the
namespaced `cglm/...` layout), so copy those headers in flat, or adjust the `#include` paths
in `src/Game.h`/`src/my_bsp.h` if you'd rather use cglm's normal folder structure.

**stb_image**: single header, public domain — used to decode material texture images
referenced from `materials/materials.txt` (see the root README for what that file does).
