#include "DepartureBoardDisplay.h"

DepartureBoardDisplay::DepartureBoardDisplay()
    : u8g2(U8G2_R0, /* clock=*/15, /* data=*/2, /* cs=*/17, /* dc=*/16,
           /* reset=*/4)
{
}

void DepartureBoardDisplay::begin() { u8g2.begin(); }

void DepartureBoardDisplay::clear()
{
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void DepartureBoardDisplay::showBoard(String stds[], String destinations[],
                                      String etds[], String callingAt[],
                                      bool cancelled[], bool delayed[],
                                      int viewIndex, int scrollOffset,
                                      bool delayPhase)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x13_tr);

    // ---- First train at row 1 ----
    int idx0 = viewIndex;
    if (idx0 < 6 && stds[idx0] != "")
    {
        String line = stds[idx0] + " " + destinations[idx0];
        int etdx = 256 - u8g2.getStrWidth(etds[idx0].c_str());
        while (u8g2.getStrWidth(line.c_str()) > etdx - 4)
            line.remove(line.length() - 1);

        u8g2.drawStr(0, 14 * 1, line.c_str());
        u8g2.drawStr(etdx, 14 * 1, etds[idx0].c_str());
    }

    // ---- Scrolling / delay / cancel line at row 2 ----
    if (idx0 < 6 && stds[idx0] != "")
    {
        if (cancelled[idx0])
        {
            if (millis() / 166 % 2 == 0)
                u8g2.drawStr(0, 14 * 2, "[!] CANCELLED");
        }
        else if (delayed[idx0] && delayPhase)
        {
            if (millis() / 333 % 2 == 0)
            {
                String msg = "[!] Delayed: " + etds[idx0];
                u8g2.drawStr(0, 14 * 2, msg.c_str());
            }
        }
        else
        {
            String prefix = "Calling at: ";
            String stops = callingAt[idx0];
            if (stops.endsWith(", "))
            {
                stops.remove(stops.length() - 2);
                stops += ".";
            }

            String scrollText = stops + "   ";
            scrollText += scrollText;

            int visibleLen = (256 - u8g2.getStrWidth(prefix.c_str())) / 6;
            if (scrollOffset >= scrollText.length())
                scrollOffset = 0;

            String view =
                scrollText.substring(scrollOffset, scrollOffset + visibleLen);

            u8g2.drawStr(0, 14 * 2, prefix.c_str());
            u8g2.drawStr(u8g2.getStrWidth(prefix.c_str()), 14 * 2,
                         view.c_str());
        }
    }

    // ---- Next train shifts into row 3 ----
    int idx1 = viewIndex + 1;
    if (idx1 < 6 && stds[idx1] != "")
    {
        String line = stds[idx1] + " " + destinations[idx1];
        int etdx = 256 - u8g2.getStrWidth(etds[idx1].c_str());
        while (u8g2.getStrWidth(line.c_str()) > etdx - 4)
            line.remove(line.length() - 1);

        u8g2.drawStr(0, 14 * 3, line.c_str());
        u8g2.drawStr(etdx, 14 * 3, etds[idx1].c_str());
    }

    // ---- Clock ----
    u8g2.setFont(u8g2_font_7x14_tr);
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", t);
    int centerX = (256 - u8g2.getStrWidth(buf)) / 2;
    u8g2.drawStr(centerX, 62, buf);

    u8g2.sendBuffer();
}