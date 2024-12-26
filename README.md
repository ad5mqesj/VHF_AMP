# Control firmware for 500W MRF300 based 2m amplifier 

Monitors forward/reverse power, Main bus DC voltage and current, transistor case temperature.  
Controls T/R relays, fans (PWM).  
Switches to transmit mode (normally) with actuve low Key In signal, provides active low Key Out signal for sequencing.  
On High (> 3.0) SWR or case Transistor case temp (> 65 C) signals fault, drops Key Out, and switches relays to byass.  

## Serial commands

Default serial parameters 115.2 kBaud, 8n1

- T/t to switch relays to transmit.
- R/r to switch relays to recieve.
- C/c to clear faults.  
