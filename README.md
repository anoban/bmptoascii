## ___Windows BMP images to ASCII strings___
--------------

_Examples in the README use JPEG version of the bitmaps because BMPs take up too much space!_

### ___Examples___
------

### ___Caveats___
-----------------
- Doesn't support any other image formats.
- Owing to the liberal reliance on `Win32` API, will not compile on UNIX systems without substantial effort.
- Not particularly good at capturing specific details in images, especially if the images are large and those details are represented by granular differences in colour gradients (this specificity gets lost in the black and white transformation and downscaling)

