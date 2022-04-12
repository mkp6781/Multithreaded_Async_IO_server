#ifndef tictactoeGame_H
#define tictactoeGame_H

#include <mutex>
#include "packet.h"

struct player{
    // ID of the player
    int player_id;

    // Symbol assigned to this particular player.
    symbol player_sym;
};

class tictactoeGame{
    // Only a single copy exists for multiple class instances.
    static int nextGameId;

public:
    // ID of the game to handle multiple games.
    int game_id;

    // Lock the game for each move.
    std::mutex m;

    // player move decider
    bool is_p1_move;

    // 3x3 game matrix
    char game_matrix[3][3];

    // ID and symbols assigned to each player.
    player player1, player2;

    // Class constructor to create an instance of a game.
    tictactoeGame(int id1, symbol sym1, int id2, symbol sym2);

    // Update the matrix after each move.
    int update_matrix(int row, int col, symbol sym);

    // Printing current state of matrix.
    void print_matrix();

    // Playing the game.
    int play_game(int pid);

    // To check the winner.
    int check_winner();
};

#endif