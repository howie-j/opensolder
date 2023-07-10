### Installation
Install STM32CubeIDE, click "Open project from filesystem" and select the "opensolder" folder.

### Code structure
All cube-generated files are untouched, except for calling opensolder_init() and opensolder_main() in main.c.
- opensolder.c is the "main" file, containing the init calls, superloop and state machine
- opensolder.h contains most constants for easy editing
- temperature.c handles interrupts, does temperature control, adc reading, tip check and such
- gui.c contains all functions to draw graphics to the OLED display

There is a fair bit of comments in the code, and better documentation can be provided if requested. If you have a question or see an issue, just open an issue in this repo.
