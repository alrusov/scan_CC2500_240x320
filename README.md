2400Mz radio diapason load scanner with sd card

See also http://forum.rcdesign.ru/f8/thread397991.html

Layout (#xx - arduino nano/micro):

Display SPI 128*160
1. VCC +3.3V
2. GND
3. CS (#9)
4. RST (#7)
5. D/C (#8)
6. MOSI (#11/16)
7. CLK (#13/15)
8. BL +3.3V

SD
1. CS (#6)
2. MOSI (#11/16)
3. MISO (#12/14)
4. CLK (#13/15)

Receiver 2,4+LNA
1. VCC +3.3V
2. MOSI (#11/16)
3. SCLK (#13/15)
4. MISO (#12/14)
5. GDO2
6. GND
7. GDO
8. CS (#10)
9. PA_EN = 0
10. LNA_EN = 1 (+3.3)


#A7/3 resistor
