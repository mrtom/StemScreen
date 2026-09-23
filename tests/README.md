# Host checks

Run the dependency-free suites from the repository root:

```sh
for suite in battery protocol ride_clock; do
  c++ -std=c++11 -Wall -Wextra -Werror -pedantic "tests/${suite}_test.cpp" -o "/tmp/stem-${suite}-test" && "/tmp/stem-${suite}-test" || exit 1
done
```

The rendering regression uses your installed Adafruit_GFX library (the same
library used by the firmware). Set its directory for your Arduino installation:

```sh
GFX_LIBRARY_DIR="$HOME/Documents/Arduino/libraries/Adafruit_GFX_Library"
c++ -std=c++11 -Wall -Wextra -Werror -pedantic -DARDUINO=100 \
  -Itests/gfx_host -I"$GFX_LIBRARY_DIR" tests/display_renderer_test.cpp \
  "$GFX_LIBRARY_DIR/Adafruit_GFX.cpp" -o /tmp/stem-display-test
/tmp/stem-display-test
```

`gfx_host` supplies only the Arduino compatibility needed by RAM canvas drawing;
it does not simulate SPI, LCD scan timing or hardware. The test compares all band
pixels to a complete offscreen frame and checks overlapping layers, icon padding,
flashing states and visible-change detection. Hardware flicker still needs a
physical check after upload.
