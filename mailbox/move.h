#pragma once
#include "promotion.h"
class Move{
    public:
        int from;
        int to;

        int movedpiece;
        int capturedpiece;
        Promotion promotion;
        bool isCastle;
        bool isEnPassant;
        bool isDoublePawnPush;

};