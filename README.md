# Alexa ESP8266 Ansulta controller
A cheep solution to control your IKEA OMLOPP and UTRUSTA lights by Amazon's Alexa.

## How to use:
1. [Buy and connect the hardware](#hardware)
2. [Build and flash the software](#software)
3. [Configure your module](#configuration)

## Hardware
- You should already have Ansulta remote Control [link](http://www.ikea.com/de/de/catalog/products/90300773/)
- Mini USB power charger.
- WeMos D1-mini from [aliexpress.com](https://de.aliexpress.com/item/D1-mini-Mini-NodeMcu-4M-bytes-Lua-WIFI-Internet-of-Things-development-board-based-ESP8266-by/32651747570.html) or similar.
- Wireless-transceiver-modul CC2500 from [aliexpress.com](https://de.aliexpress.com/item/Wireless-Module-CC2500-2-4G-Low-power-Consistency-Stability-Small-Size/32702148262.html).
- If you want it modular, you can use adapter boards. But you can connect the ESP8266 and CC2500 directly.

![My module top](https://github.com/atiderko/esp8266-ansulta-alexa/blob/master/my_module_top.jpg)![My module bottom](https://github.com/atiderko/esp8266-ansulta-alexa/blob/master/my_module_bottom.jpg)![Connection scheme](https://github.com/atiderko/esp8266-ansulta-alexa/blob/master/scheme.png)

## Software
- Arduino IDE from [Arduino website](http://www.arduino.cc/en/main/software)
- [Arduino core for ESP8266 WiFi chip](https://github.com/esp8266/Arduino) - I installed v2.5.0 from Git 
- [WiFiManager](https://github.com/tzapu/WiFiManager/) - I use v0.14 from Git
- this repository
- Build and flash the ESP8266. Select - Board:"LONIN(WeMoS) D1 R2 & mini", Upload Speed: "115200", Flash Size: 4M(1M SPIFFS)

## Configuration
1. On first start an AP for web configuration portal is launched with SSID "AnsultaAP". Take your smartphone to connect to this AP. Once connected you can select your WiFi enter password and name for device shown in Alexa.
2. First option: name of the lamp, second option: timeout for motion detection (0: disable), third option: light intensity if motion (PIN: D0) and light (PIN: A0) sensor connected. 
3. After the module is successful connected to WiFi you have to push the button (several times if needed) on your Ansulta Remote Control.
4. If the address was learned the modules tries to switch them in follow order: 50% - 1s - 100% - 1s - 50% - 1s - OFF.
> The LED on ESP should be now off and not blink!
5. Let search Alexa for Smart Home devices. I use it with Amazon Echo Plus.
6. That's all. You can now switch and dim (50% and 100%) the lights in your kitchen by Alexa.

>If WiFi is not connected or Ansulta address not learned the LED glow or blink. You can reset the settings by pushing the reset button twice. The ESP starts again with AP for web configuration portal. Repeat all configuration steps again.

## References to used code
- https://github.com/NDBCK/Ansluta-Remote-Controller
- https://github.com/probonopd/ESP8266HueEmulator
- https://github.com/datacute/DoubleResetDetector

## Thanks for support
- [@JoTid](https://github.com/JoTid)
- [paypal.me/atiderko](https://www.paypal.me/atiderko)
