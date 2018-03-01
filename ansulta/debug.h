#include <SPI.h>

#define DEBUG //"Schalter" zum aktivieren

// do not change
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_FPRINT(x, y) Serial.print(x, y)
#define DEBUG_FPRINTLN(x, y) Serial.println(x, y)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_FPRINT(x, y)
#define DEBUG_FPRINTLN(x, y)
#endif

