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
#ifndef HUEWCFNREQUESTHANDLER_H
#define HUEWCFNREQUESTHANDLER_H

#include <ESP8266WebServer.h>

namespace hue {

class WcFnRequestHandler;

typedef std::function<void(WcFnRequestHandler *handler, String requestUri, HTTPMethod method)> WcFnHandlerFunction;

class WcFnRequestHandler : public RequestHandler {
public:
    WcFnRequestHandler(WcFnHandlerFunction fn, const String &uri, HTTPMethod method, char wildcard = '*');
    bool canHandle(HTTPMethod requestMethod, String requestUri) override;
    bool canUpload(String requestUri) override;
    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override;
    void upload(ESP8266WebServer& server, String requestUri, HTTPUpload& upload) override;
    String getWildCard(int wcIndex);
protected:
    String currentReqUri;
    WcFnHandlerFunction _fn;
    String _uri;
    HTTPMethod _method;
    char _wildcard;
    
    String removeSlashes(String uri);
    String getPathSegment(String uri);
    String removePathSegment(String uri);
    String getWildCard(String requestUri, String wcUri, int n, char wildcard = '*');
    
};


};
#endif
