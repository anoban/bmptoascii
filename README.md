# ___`Windows BMP images to ASCII strings`___

### ___Examples___
------

### ___Caveats___
-----------------
- Doesn't support any other image formats.
- Has hard dependencies on `Win32`, hence non-portabe to non-Windows systems.
- Not particularly good at capturing specific details in images, especially if those details are represented by minute differences in colour gradients (this specificity gets lost in the black and white transformation and downscaling)
- The outputs shown in the `README` aren't very representative, best samples are chosen for showcasing, some ASCII representations often turn out without any semblance to the original image!
