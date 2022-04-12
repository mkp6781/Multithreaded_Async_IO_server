#ifndef PACKET_H
#define PACKET_H

# include <string>

enum msg_types{
    SERVER_REQUEST_MOVE,
    REPLAY_GAME,
    PLAYER_MOVE,
    OPPONENT_MOVE,
    MOVE_SUCCESS,
    WIN,
    LOSS,
    DRAW,
    REPLAY,
    RESTART_GAME,
    END_GAME,
    PARTNER_DISCONNECT,
};

enum symbol{
    EMPTY,
    CROSS,
    CIRCLE,
    TIMEOUT,
    DISCONNECT
};

#pragma pack(1)   // helps to pack the struct to 16-bytes
struct tictactoe_packet{
    msg_types msg_type;
    symbol player_sym;
    int row, col;
};
#pragma pack(0)   // turn packing off

#endif