# DoMacroC

*Note: You need the program running in the background.*

Syntax
```
echo "COMMAND ARGS" > /tmp/domacro
```

Example
```
echo "unique stardewvally keyHold 46 100 sleep 100 keyPress 54 keyPress 111 keyPress 19 sleep 80 keyRelease 54 keyRelease 111 keyRelease 19" > /tmp/domacro
```
The wait times are in milliseconds.

The unique keyword will not allow two instances with the same name to run at the same time.
Currently use keycodes instead of keynames. Will be fixed in the future.

## Build
```
make
```
