# Not yet verified

0.3.1 ran on an Akai Force. 0.6.1 has been built for the Force and passes the installer and catalog checks, but nothing in it has
run on a Force yet. In particular:

* Every knob turns in place and the 16 step faders draw and move (the strips are the same size and frame count as in 0.3.1).
* The Force sends tempo and song position to the plugin (the band at the top of the page shows what MORI receives).
* The step modulators chop and move sound in time; the DUB loop records and plays on the bar.
* Loading a preset moves the knobs on the screen; SAVE NEW keeps a preset after the project is reloaded.
* The touch bar is drawn and answers a finger (its log is `pad/touch.log` in the plugin folder).
* The browser editor opens from the address the EDITOR button shows, and controls follow in both directions.
* CPU use has not been measured on the Force.
* The pictures of the pages after ROUTING in the README are layout pictures made from the skin files, not photos of a Force.
