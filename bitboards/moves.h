#pragma once

#include <cstdint>
#include<iostream>
#include <string>
#include"bitboards.h"
#include "board.h"
struct Move{
    uint32_t moveInfo;

    static constexpr uint32_t fromMask=(1u<<6)-1u;
    static constexpr uint32_t toMask=((1u<<6)-1u)<<6;
    static constexpr uint32_t movedPieceMask=((1u<<4)-1u)<<12;
    static constexpr uint32_t capturedPieceMask=((1u<<4)-1u)<<16;
    static constexpr uint32_t promotionPieceMask=((1u<<4)-1u)<<20;
    static constexpr uint32_t isCaptureMask=(1u<<24);
    static constexpr uint32_t isDoublePawnPushMask=(1u<<25);
    static constexpr uint32_t isEnPassantMask=1u<<26;
    static constexpr uint32_t isCastlingMask=1u<<27;
};

inline Move makeMove(uint32_t from,
    uint32_t to,
    uint32_t movedPiece,
    uint32_t capturePiece=0,
    uint32_t promotionPiece=0,
    bool isCapture=false,
    bool isDoublePawnPush=false,
    bool isEnPassant=false,
    bool isCastle=false){
        return Move{(from & 0x3F) | (to & 0x3F) <<6 | (movedPiece & 0xF) << 12 | ((capturePiece & 0xF) << 16) | (promotionPiece & 0xF) << 20 |
         (uint32_t)isCapture <<24 | (uint32_t)isDoublePawnPush << 25 | (uint32_t)isEnPassant << 26 | (uint32_t)isCastle << 27};
    }

inline int getFrom(const Move& move){
    return move.moveInfo & Move::fromMask;
}
inline int getTo(const Move& move){
    return (move.moveInfo & Move::toMask) >> 6;
}
inline int getMovedPiece(const Move& move){
    return (move.moveInfo & Move::movedPieceMask)>>12;
}
inline int getCapturedPiece(const Move& move){
    return (move.moveInfo & Move::capturedPieceMask)>>16;
}
inline int getPromotionPiece(const Move& move){
    return (move.moveInfo & Move::promotionPieceMask)>>20;
}
inline bool isCastle(const Move& move){
    return (move.moveInfo & Move::isCastlingMask) >>27;
}
inline bool isEnPassant(const Move& move){
    return (move.moveInfo & Move::isEnPassantMask)>>26;
}
inline bool isDoublePawnPush(const Move& move){
    return (move.moveInfo & Move::isDoublePawnPushMask)>>25;
}
inline bool isCapture(const Move& move){
    return (move.moveInfo & Move::isCaptureMask)>>24;
}
inline std::string printMove(const Move& move){
    return squareToString(getFrom(move))+ squareToString(getTo(move));
}
