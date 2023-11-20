# ___`A minimal ASCII art generator`___

---------------
### ___Prints out the ASCII image of a `Windows BMP` image file___
-------------

#### ___Low Resolution Samples___
![Circle](./media/circle.bmp)


![Wine Glass](./media/wineglass.bmp) :=>
![ASCII](./wineglass.png)

![Vendetta](./media/vendetta.bmp)


#### ___High Resolution Samples___
![Hat](./media/highres/hat.bmp)


![Blonde](./media/highres/blonde.bmp)

### ___Caveats___
-----------------
- Doesn't support any other image formats.
- `Win32` oriented, portability is a non-goal.
- Most routines are implemented as inline functions in the headers.
- Images (ASCII arts) often end up a little distorted, in particular, they tend to become narrow and tall compared to the original `.BMP` image.
