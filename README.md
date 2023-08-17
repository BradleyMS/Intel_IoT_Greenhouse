# Intel_IoT_Greenhouse
Arduino IDE code to control the Intel IoT Greenhouse via a web portal

This code allows manual control of the LEDs and a mist maker on the Intel IoT Greenhouse, which runs on an Intel Galileo Gen 2 dev board.  It also displays the temperature & motion sensor values.

The motion sensor automatically opens the front door, and the temperature sensor automatically turns on the fan @ 85 degrees and opens the windows @ 90 degrees, so these are not manually controlled in the web portal, but could be added.

In order to program this board, I had to install the "Intel i586 Boards" from the boards manager.

In order to control the LEDs I had to add the LPD8806 library and modify it to allow for a missing "F_CPU" value to work with this board.

Version 2.2 and above includes controls for a 10w LED grow light on relay 2.  The light will automatically turn on, and then automatically turn off at 90F
