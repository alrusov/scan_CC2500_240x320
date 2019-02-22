2400Mz radio diapason load scanner

See also http://forum.rcdesign.ru/f8/thread397991.html

Layout (#xx - arduino nano pins):

Display SPI 128*160
1. VCC +3.3V
2. GND
3. CS (#9)
4. RST (#7)
5. D/C (#8)
6. MOSI (#11)
7. CLK (#13)
8. BL +3.3V

Receiver 2,4+LNA
1. VCC +3.3V
2. MOSI (#11)
3. SCLK (#13)
4. MISO (#12)
5. GDO2
6. GND
7. GDO
8. CS (#10)
9. PA_EN = 0
10. LNA_EN = 1 (+3.3)

Receiver 2,4 (small)
1. GND
2. VCC +3.3V
3. MOSI (#11)
4. SCLK (#13)
5. MISO (#12)
6. GDO2
7. GDO0
8. CSN (#10)

#A7 resistor
