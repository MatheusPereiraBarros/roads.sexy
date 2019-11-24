# roads.sexy


All of the code for converting the 2d Map data into 3d .obj files and other information that is used by Unity to create e.g. street lamps
is done here:

https://github.com/Matthiaas/roads.sexy/blob/master/hackatum-2019/src/xodr_viewer/xodr_viewer_window.cpp

All the C# implementation is done here: 

https://github.com/Matthiaas/roads.sexy/tree/master/unity_rendering/Assets
 
All of the visible stuff is loaded at runtime with some scripst. There is nobody actually touching Unity and putting gameobject into the s
cene.
Everything that is requiered a camera and a gameobject that starts our script.
