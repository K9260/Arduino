## VU-meter
#### Includes file for VU-meter that automatically calculates average volume, so no manual adjustment is required.

## VU-meter-rotary
#### Almost the same file as VU-meter, but with manual sensitivity adjustment and LCD screen.

***

For VU-meter what you need to do is to insert microphone Vcc to 3.3V, GND TO GND and data pin to A0.  
If you are not using arduino, you must define the data pin yourself, it just has to be analog input.  
There is also button now defined for changing effects. At the moment I do not have schematic to share for it.  
Then all you need to adjust in the code is NUM_LEDS.  
Few variables you might also want to play with are: 
<strong>
* MAXBEATS
* MAXBUBBLES
* MAXRIPPLES
* MAXTRAILS
</strong>

### Video of my build:
[![](http://img.youtube.com/vi/Box-O3KY1qI/0.jpg)](http://www.youtube.com/watch?v=Box-O3KY1qI "Youtube")
