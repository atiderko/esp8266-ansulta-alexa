/**************************************************************

This file is a part of
https://github.com/atiderko/esp8266-ansulta-alexa
Copyright (c) 2018 Alexander Tiderko

Licensed under MIT license

Simulates the Hue Bridge which can be found and control by Alexa.
The code is based on
https://github.com/probonopd/ESP8266HueEmulator
(removed dependency from aJSON, NtpClient, Time, NeoPixelBus)

**************************************************************/
#ifndef HUETEMPLATES_H
#define HUETEMPLATES_H

namespace hue {
  
// Group and Scene file name prefixes for SPIFFS support 
String GROUP_FILE_TEMPLATE = "GROUP-%d.json";
String SCENE_FILE_TEMPLATE = "SCENE-%d.json";


static const char* SSDP_RESPONSE_TEMPLATE =
  "HTTP/1.1 200 OK\r\n"
  "EXT:\r\n"
  "CACHE-CONTROL: max-age=%u\r\n" // SSDP_INTERVAL
  "LOCATION: http://%s:%u/%s\r\n" // WiFi.localIP(), _port, _schemaURL
  "SERVER: FreeRTOS/6.0.5, UPnP/1.0, %s/%s\r\n" // _modelName, _modelNumber
  "hue-bridgeid: %s\r\n"
  "ST: %s\r\n"  // _deviceType
  "USN: uuid:%s::upnp:rootdevice\r\n" // _uuid::_deviceType
  "\r\n";

static const char* SSDP_NOTIFY_TEMPLATE =
  "NOTIFY * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "NTS: ssdp:alive\r\n"
  "CACHE-CONTROL: max-age=%u\r\n" // SSDP_INTERVAL
  "LOCATION: http://%s:%u/%s\r\n" // WiFi.localIP(), _port, _schemaURL
  "SERVER: FreeRTOS/6.0.5, UPnP/1.0, %s/%s\r\n" // _modelName, _modelNumber
  "hue-bridgeid: %s\r\n"
  "NT: %s\r\n"  // _deviceType
  "USN: uuid:%s::upnp:rootdevice\r\n" // _uuid
  "\r\n";
  
static const char* SSDP_XML_TEMPLATE = "<?xml version=\"1.0\" ?>"
  "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
  "<specVersion><major>1</major><minor>0</minor></specVersion>"
  "<URLBase>http://{ip}:{port}/</URLBase>"
  "<device>"
    "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
    "<friendlyName>Philips hue ({ip})</friendlyName>"
    "<manufacturer>Royal Philips Electronics</manufacturer>"
    "<manufacturerURL>http://www.philips.com</manufacturerURL>"
    "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
    "<modelName>Philips hue bridge 2012</modelName>"
    "<modelNumber>929000226503</modelNumber>"
    "<modelURL>http://www.meethue.com</modelURL>"
    "<serialNumber>{mac}</serialNumber>"
    "<UDN>uuid:2f402f80-da50-11e1-9b23-{mac}</UDN>"
    "<presentationURL>index.html</presentationURL>"
    "<iconList>"
    "  <icon>"
    "    <mimetype>image/png</mimetype>"
    "    <height>48</height>"
    "    <width>48</width>"
    "    <depth>24</depth>"
    "    <url>hue_logo_0.png</url>"
    "  </icon>"
    "  <icon>"
    "    <mimetype>image/png</mimetype>"
    "    <height>120</height>"
    "    <width>120</width>"
    "    <depth>24</depth>"
    "    <url>hue_logo_3.png</url>"
    "  </icon>"
    "</iconList>"
  "</device>"
  "</root>";
};
#endif
