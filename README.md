# SMPTE-LTC-Decoder
Microchip PIC based LTC decoder

Author: Camillo Sacco

This project is based on Microchip PIC16F15313 and strictly relies on its CLC and NCO modules.
Balanced analog audio is fed inside the two CMP pins 6,7 (C1IN0-, C1IN0+).
Appropriate analog circuitry is required to adapt signal levels and protect the microcontroller input.

SMPTE LTC uses BMC (biphase mark code) wich is very similar to Manchester encoding.
With reference to the Microchip application note <a href="https://ww1.microchip.com/downloads/en/Appnotes/01470A.pdf">AN1470</a> ("Manchester Decoder Using the CLC and NCO") and few adaptations and optimizations we are able to decode the BMC in the same way, all in hardware, thanks to four CLC and one NCO module.

Decoded data and decoded clock are at the same time available on two external pins (2-data, 5-clock) and fed to a free running SPI client module.
SPI module is used to read data 8bit per time. The drawback is that SPI module is not in sync with the beginning of the frame.
In firmware we look for the LTC sync word (0b0011111111111101) and then shift all the data properly.
Data is then sent thru serial module (pin 3, 115200-8N1) in ascii format "HH:mm:ss:ff\n"

<img src="https://github.com/as-cmi/SMPTE-LTC-Decoder/raw/master/DOC/pinout.png"></img>
