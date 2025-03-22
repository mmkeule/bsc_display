# Statusdisplay für den Battery safety controller (BSC) 
Dies ist ein angepasster Fork des Repository von shining-man um die Palette der Panlee Displays nutzen zu können

<img src="https://github.com/user-attachments/assets/8db86295-1d72-4389-9650-80bd504366ac" height="500"/>
<br><br>
Das Display zeigt den Status des Battery safety controllers (BSC).<br>
Als Display wird ein WT32-SC01 genutzt. Voraussetzung für das Display ist ein Softwarestand vom BSC >= V0.2.0.

* Daten von den verbundenen BMSen (5x Bluetooth, 3x Serial)
  * Spannung, Strom, SoC
  * Zellspannungen
  * Balancer ein/aus, Fehler
* Wechselrichterdaten (CAN)
  * Spannung, Strom, SoC
  * Max. Lade/Entlade Strom
* Status der Alarme vom BSC

## Flashen des ESP32
Das Flashen kann nach folgender Beschreibung durchgeführt werden: 
[Flashen des ESP32](https://github.com/shining-man/bsc_display/wiki/02-Flashen-des-Displays)

## Verbinden des Displays mit dem BSC
Verbunden wird das Display über den I2C-Bus mit dem BSC.<br>
Der I2C-Bus ist je nach PCB Version des BSC auf folgenden Steckern zu finden:<br>
PCB V1.0: X1<br>
PCB v2.0-v2.2: Extension-Port J11 (2x3 pol. Pfostenleiste)<br>
PCB ab v2.3: Extension-Port J3 (2x5 pol. Pfostenleiste)<br>
<br>
Am Display ist der I2C-Bus auf folgenden GPIOs:<br>
SDA: GPIO32<br>
SCL: GPIO33<br>
<br>
![Picture_BSC_Display_BMS](https://github.com/shining-man/bsc_display/blob/main/img/Disp_BMS.jpg?raw=true)
![Picture_BSC_Display_Cellspgannungen](https://github.com/shining-man/bsc_display/blob/main/img/Disp_Cell.jpg?raw=true)

## BOARD-Datei WT32.json
Bitte die Boarddatei im Benutzerordner z.B.: C:\Users\Username\.platformio\platforms\espressif32\boards platzieren. 
Darauf achten welche Platform genutzt wird

## PanelLan Library
in der Display.cpp kann nun in Zeile 13 Das gewünschte Board ausgewählt werden. Bitte beachten, dass auch die i2C Pins
entsprechend angepasst werden müssen. Dies geht in der Datei include/defines.h
