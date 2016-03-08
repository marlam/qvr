# qvr-sceneviewer - QVR viewer for a wide variety of models and scenes

qvr-sceneviewer imports models and scenes using the
[Open Asset Import Library](http://assimp.org/), and renders them.
[Many file formats](http://assimp.org/main_features_formats.html) are supported.

Features:
* Fast rendering using an OpenGL core context.
* Support for spot lights, point lights, and directional lights in the scene.
* Materials can have ambient maps, diffuse maps, specular maps, shininess maps,
  opacity maps, ambient occulusion maps, bump maps, and normal maps.

Current limitations (these might be removed in future versions):
* Only one set of texture coordinates can be used.
* Animations are not currently supported.

Scenes can be pretransformed with a matrix defined on the command line:
* `-r angle,x,y,z` or `--rotate angle,x,y,z`:  
  Rotate `angle` degrees around vector `(x,y,z)`.
* `-s s` or `--scale s` or `-s sx,sy,sz` or `--scale sx,sy,sz`:  
  Scale coordinates with `s` or with `sx,sy,sz`.
* `-t x,y,z` or `--translate x,y,z`:  
  Translate by offset `(x,y,z)`.
These transformations will be applied to the pretransformation matrix in the
order in which they are specified on the command line.

Freely available scenes to try, with appropriate pretransformation:
* Crytek Sponza from [McGuire Graphics Data](http://graphics.cs.williams.edu/data/meshes.xml):  
  `qvr-sceneviewer --scale=0.01 --rotate=90,0,1,0 --translate=0,0,70 crytek-sponza/sponza.obj`
* Rungholt from [McGuire Graphics Data](http://graphics.cs.williams.edu/data/meshes.xml):  
  `qvr-sceneviewer --translate=0,1,0 --scale=0.1 rungholt/rungholt.obj`
