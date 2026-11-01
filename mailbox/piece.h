#pragma once
namespace Piece {
    constexpr int None   = 0;
    constexpr int King   = 1;
    constexpr int Pawn   = 2;
    constexpr int Knight = 3;
    constexpr int Bishop = 4;
    constexpr int Rook   = 5;
    constexpr int Queen  = 6;
    
    constexpr int White = 8;
    constexpr int Black = 16;
    
    constexpr int TypeMask = 0b00111;
    constexpr int ColorMask = 0b11000;
}