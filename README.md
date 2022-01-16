# IndoorAirQuality

Wiring described in wiring.ino
I used Firebeetle ESP32-E so wiring is according to that. 

Uses DHT22 and CCS811 to detect and display air quality in the room.
Need to get a baseline for CCS811 first from https://github.com/Sindar-sudo/CSS811getbaseline
ST7735 0.96" RGB screen was used here, but it's adjustible to other screens too via files in TFTConfig (these are not my files, these are from https://github.com/Bodmer/TFT_eSPI/ )
My screen had strange colors, it was not rly RGB, but via experimentation I found the correct colors for it. Yours may be different.
