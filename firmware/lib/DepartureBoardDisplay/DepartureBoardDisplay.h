#ifndef DEPARTURE_BOARD_DISPLAY_H
#define DEPARTURE_BOARD_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>

class DepartureBoardDisplay
{
   public:
    DepartureBoardDisplay();
    void begin();
    void showBoard(String stds[], String destinations[], String etds[],
                   String callingAt[], bool cancelled[], bool delayed[],
                   int viewIndex, int scrollOffset, bool delayPhase);
    void clear();

   private:
    U8G2_SH1122_256X64_F_4W_SW_SPI u8g2;
};

#endif
