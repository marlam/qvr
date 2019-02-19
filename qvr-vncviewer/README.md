# qvr-vncviewer - VNC Viewer for QVR

qvr-vncviewer is a VNC viewer for QVR.
You can use it to view VNC screens across multiple displays, graphics cards,
or hosts, or inside virtual reality devices such as the Oculus Rift.

qvr-vncviewer places the VNC screen inside the virtual world. The screen
geometry can currently be a wall (rectangle) or part of a cylinder, specified
with the following options:

* `--wall blx,bly,blz,brx,bry,brz,tlx,tly,tlz`: 
  Draw a rectangular VNC screen, given by the bottom left, bottom right, and
  top left corners of a rectangular wall.
* `--cylinder cx,cy,cz,ux,uy,uz,r,phi0,phirange,thetarange`:
  Draw a cylindrical VNC screen, given by the cylinder center, up vector,
  radius, azimuth angle of screen center (phi0), and the aperture angles for
  azimuth (phirange) and polar (thetarange) directions.

qvr-vncviewer uses [libvncclient](http://libvncserver.sourceforge.net/) and
therefore supports the usual VNC options and arguments. For example:
$ qvr-vncviewer --wall -1,-1,-3,+1,-1,-3,-1,+1,-3 \
                --qvr-config myconf.qvr \
		-compress 9 myvncserver:0

qvr-vncviewer is a viewer only; keyboard and mouse interaction are currently
not supported.
