#pragma once
#include <ncurses.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

class BrailleChart {
public:
    static void drawBoxChart(WINDOW *win, int y, int x, int h, int w,
                             const std::vector<double> &history, int colorPair,
                             const std::string &topLeft = "", const std::string &topRight = "") {
        if (h < 3 || w < 6) return;
        wattron(win, COLOR_PAIR(6));
        mvwaddwstr(win, y, x, L"┌");
        for (int i = 1; i < w - 1; ++i) mvwaddwstr(win, y, x + i, L"─");
        mvwaddwstr(win, y, x + w - 1, L"┐");
        for (int r = 1; r < h - 1; ++r) { mvwaddwstr(win, y + r, x, L"│"); mvwaddwstr(win, y + r, x + w - 1, L"│"); }
        mvwaddwstr(win, y + h - 1, x, L"└");
        for (int i = 1; i < w - 1; ++i) mvwaddwstr(win, y + h - 1, x + i, L"─");
        mvwaddwstr(win, y + h - 1, x + w - 1, L"┘");
        wattroff(win, COLOR_PAIR(6));

        if (!topLeft.empty()) {
            wattron(win, A_BOLD);
            mvwaddnstr(win, y, x + 2, (" " + topLeft + " ").c_str(), w - 4);
            wattroff(win, A_BOLD);
        }
        if (!topRight.empty() && static_cast<int>(topRight.size()) + 4 < w)
            mvwaddnstr(win, y, x + w - static_cast<int>(topRight.size()) - 3, (" " + topRight + " ").c_str(), w - 4);

        int innerH = h - 2, innerW = w - 2, dotH = innerH * 4, dotW = innerW * 2;
        std::vector<double> samples(dotW, 0.0);
        int histSz = static_cast<int>(history.size());
        for (int dx = 0; dx < dotW; ++dx) {
            int idx = histSz - dotW + dx;
            if (idx >= 0 && idx < histSz) samples[dx] = std::clamp(history[idx], 0.0, 100.0);
        }
        const std::vector<int> lB = {0x01, 0x02, 0x04, 0x40}, rB = {0x08, 0x10, 0x20, 0x80};
        wattron(win, COLOR_PAIR(colorPair));
        for (int row = 0; row < innerH; ++row) {
            for (int col = 0; col < innerW; ++col) {
                int mask = 0;
                int fL = static_cast<int>(std::round((samples[col * 2] / 100.0) * dotH));
                int fR = static_cast<int>(std::round((samples[col * 2 + 1] / 100.0) * dotH));
                for (int sub = 0; sub < 4; ++sub) {
                    int dy = (innerH - 1 - row) * 4 + (3 - sub);
                    if (dy < fL) mask |= lB[sub];
                    if (dy < fR) mask |= rB[sub];
                }
                std::wstring ws(1, static_cast<wchar_t>(0x2800 + mask));
                mvwaddwstr(win, y + 1 + row, x + 1 + col, ws.c_str());
            }
        }
        wattroff(win, COLOR_PAIR(colorPair));
    }
};
