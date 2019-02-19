# qvr-sceneviewer - QVR viewer for a wide variety of models and scenes

qvr-sceneviewer imports models and scenes using the
[Open Asset Import Library](http://assimp.org/), and renders them.
[Many file formats](http://assimp.org/main_features_formats.html) are supported.

Features:

* Fast rendering using an OpenGL core context.
* Support for spot lights, point lights, and directional lights in the scene.
* Materials can have ambient maps, diffuse maps, specular maps, emissive maps,
  shininess maps, opacity maps, lightness maps (e.g. for ambient occlusion),
  bump maps, and normal maps.

Current limitations (these might be removed in future versions):

* Only one set of texture coordinates can be used.
* Only simple transparencies are supported; no blending of semi-transparent
  materials takes place.
* Animations are not currently supported.

Scenes can be transformed with a matrix defined by rotate, scale, and translate
operations given on the command line:

* `-r angle,x,y,z` or `--rotate angle,x,y,z`:  
  Rotate `angle` degrees around vector `(x,y,z)`.
* `-s s` or `--scale s` or `-s sx,sy,sz` or `--scale sx,sy,sz`:  
  Scale coordinates with `s` or with `sx,sy,sz`.
* `-t x,y,z` or `--translate x,y,z`:  
  Translate by offset `(x,y,z)`.

Freely available scenes to try, with appropriate transformation:

* Crytek Sponza from [McGuire Graphics Data](http://graphics.cs.williams.edu/data/meshes.xml):  
  `qvr-sceneviewer --scale=0.01 --rotate=90,0,1,0 --translate=0,0,70 crytek-sponza/sponza.obj`
* Rungholt from [McGuire Graphics Data](http://graphics.cs.williams.edu/data/meshes.xml):  
  `qvr-sceneviewer --scale=0.5 --translate=0,-6,0 rungholt/rungholt.obj`
