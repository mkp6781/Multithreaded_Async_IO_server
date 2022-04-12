#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include "tictactoe.h"

tictactoeGame::tictactoeGame(int id1, symbol sym1, int id2, symbol sym2){
    ++nextGameId;
    game_id = nextGameId;

    // Assigning player parameters to the current game.
    player1.player_id = id1;
    player2.player_id = id2;
    player1.player_sym = sym1;
    player2.player_sym = sym2;

    // Allowing player1 to make the first move.
    is_p1_move = true;

    // Initialising game matrix.
    memset(game_matrix, EMPTY, sizeof(game_matrix));
}

int tictactoeGame::update_matrix(int row, int col, symbol sym){
    if(row<0 || row>2 || col<0 || col>2){
        return -1;
    }
    if(game_matrix[row][col]==EMPTY){
        game_matrix[row][col] = sym;
        return 0;
    }
    return -1;
}

void tictactoeGame::print_matrix(){
    for(int i=0;i<3;++i){
        for(int j=0;j<3;++j){
            std::cout<<game_matrix[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
}

int tictactoeGame::check_winner(){
    int winner;
    // any of the rows or columns is same
    for(int i=0;i<=2;i++){
        if(game_matrix[i][0]==game_matrix[i][1] && game_matrix[i][1]==game_matrix[i][2] && game_matrix[i][0]!=EMPTY){
            winner = game_matrix[i][0];
            std::cout<<"Winner Row."<<std::endl;
            return winner;
        }
        if(game_matrix[0][i]==game_matrix[1][i] && game_matrix[1][i]==game_matrix[2][i] && game_matrix[0][i]!=EMPTY){
            winner = game_matrix[0][i];
            std::cout<<"Winner Column."<<std::endl;
            return winner;
        }
    }

    // Check diagonals are same
    if(game_matrix[0][0]==game_matrix[1][1] && game_matrix[1][1]==game_matrix[2][2] && game_matrix[0][0]!=EMPTY){
        winner = game_matrix[0][0];
        std::cout<<"Winner Diagonal"<<std::endl;
        return winner;
    }
    if(game_matrix[0][2]==game_matrix[1][1] && game_matrix[1][1]==game_matrix[2][0] && game_matrix [0][2]!=EMPTY){
        winner = game_matrix[0][2];
        std::cout<<"Winner Off Diagonal"<<std::endl;
        return winner;
    }

    // Neither of the players have won yet. So we check if the board has any cell left.
    // If any cell is still EMPTY, the game continues.
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(game_matrix[i][j]==EMPTY){
                winner = -1;
                return winner;
            }
        }
    }

    // all boxes full. The game is drawn.
    winner = 0;
    std::cout<<"Draw"<<std::endl;
    return winner;
}