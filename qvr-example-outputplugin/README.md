# qvr-example-outputplugin - Example output plugin for QVR

QVR supports per-window output plugins. An output plugin takes textures
containing one or two views of the scene as input and displays them on the
screen, applying any kind of postprocessing that might be necessary.

This is useful if your VR hardware uses GPU-based blending, masking, warping,
or color correction, or if you use a custom stereo-3D setup that requires
special processing. You can build an output plugin for these purposes and
specify it in the configuration file. Your applications do not need to know
about these things.

This toy example provides two simple screen space effects: a ripple effect and
an edge detection effect. These can be activated by arguments passed to the
plugin via the configuration file. See `configs/4-window-outputplugin.qvr`.
