# ___`A minimal ASCII art generator`___

---------------
### ___Prints out the ASCII text representation of a `Windows BMP` image files___
-------------

#### ___Examples___

<div><img src="./readme/bulb_.jpg" height=500 width=400> <img src="./readme/bulb.jpg" height=500 width=400></div><br>
<div><img src="./readme/candle_.jpg" height=500 width=400> <img src="./readme/candle.jpg" height=500 width=400></div><br>
<div><img src="./readme/wineglass_.jpg" height=500 width=400> <img src="./readme/wineglass.jpg" height=500 width=400></div><br>
<div><img src="./readme/vendetta_.jpg" height=500 width=400> <img src="./readme/vendetta.jpg" height=500 width=400></div><br>

### ___Caveats___
-----------------
- Doesn't support any other image formats.
- Has hard dependencies on `Win32`, portability is a non-goal.
- Most routines are implemented as inline functions in the headers.
- Not particularly good at capturing specific details in images, especially if those details are represented by minute differences in colour gradients (this specificity gets lost in the black and white transformation and downscaling)
- The outputs shown in the `README` aren't very representative, best samples are chosen for showcasing, some ASCII representations often turn out without any semblance to the original image!
