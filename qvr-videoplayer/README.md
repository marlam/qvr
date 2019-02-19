# qvr-videoplayer - A video screen in Virtual Reality

qvr-videoplayer renders a video screen in the virtual environment.
It is based on the Qt Multimedia module and has no further requirements.

Features:

* Arbitrary video screen geometry
* Support for 2D and stereoscopic 3D videos
* Support for multi-GPU/multi-process configurations

## Usage

The list of videos to play is given on the command line:
`qvr-videoplayer video1.mp4 video2.mp4 http://example.com/video3.wmv`

Options:

* `--loop`: loop the playlist
* `--screen`: set the video screen geometry; see next section

Keyboard shortcuts:

* Media keys work as expected
* Space: pause/play
* P/N: jump to previous/next video in playlist
* M: toggle audio mute
* Left/Right: seek 10 seconds backward/forward
* Down/Up: seek 1 minute backward/forward
* PageDown/PageUp: seek 10 minutes backward/forward

## The video screen

By default, `qvr-videoplayer` places a 16:9 video screen in the virtual world.
This is useful for HMD-based Virtual Reality systems (HTC Vive, Oculus Rift,
...).

If instead you have a Virtual Reality setup based on a fixed display screen,
you want the virtual video screen geometry to match your physical display
geometry.

In case of a planar physical display screen, simply specify the 3D coordinates
of its bottom left, bottom right, and top left corners:
`qvr-videoplayer --screen=blx,bly,blz,brx,bry,brz,tlx,tly,tlz`

If you have a non-planar physical screen, e.g. a half-cylinder, you can model
your physical screen geometry in an OBJ file, with appropriate texture
coordinates. For such arbitrary video screen geometry, an aspect ratio needs to
be specified manually. Example:
`qvr-videoplayer --screen=16:9,myscreen.obj`

## Stereoscopic 3D video

Stereoscopic 3D videos provide a left eye view and a right eye view. `qvr-videoplayer`
supports video files that combine both views in each video frame. How the views
are arranged in a frame can not always be detected automatically. You can specify
the arrangement in the file name if autodetection fails:

* `video-lr.mp4`: left view in left half of frame, right view in right half
* `video-rl.mp4`: left view in right half of frame, right view in left half
* `video-lrh.mp4`: left view in left half of frame, right view in right half, both squeezed to half width
* `video-rlh.mp4`: left view in right half of frame, right view in left half, both squeezed to half width
* `video-tb.mp4`: left view in top half of frame, right view in bottom half
* `video-bt.mp4`: left view in bottom half of frame, right view in top half
* `video-tbh.mp4`: left view in top half of frame, right view in bottom half, both squeezed to half height
* `video-bth.mp4`: left view in bottom half of frame, right view in top half, both squeezed to half height
* `video-2d.mp4`: 2D video
