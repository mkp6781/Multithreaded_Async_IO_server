#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <iostream>
#include <logger.h> // For logging.
#include <map>
#include <memory>
#include <netinet/in.h> // For sockaddr_in
#include <string.h>
#include <sys/socket.h>
#include <thread> //for threading , link with lpthread
#include <tictactoe.h>
#include <unistd.h> // For read

using namespace std;

#define SERVER_PORT 8911
#define SERVER_BACKLOG 5
#define NOONE -1

enum RESTART{
    NO_RESPONSE,
    YES,
    NO
};

int serverfd , clientfd, curr_num_of_connections = 0;

// Logger Variables
Log logger;
string fail_msg, success_msg, log_msg;

struct timespec start_time, end_time; // Time calculation variables
map<int, int> partners; // Stores the game partners for each game.
int tictactoeGame::nextGameId = 0; // Keeps incrementing the game id for each new game.
int id_player_waiting = NOONE; // The socket descriptor of player who is waiting in the lobby for an opponent.
map<int, tictactoeGame*> games; // Stores the pointer to the game instance corresponding to each player.
map<tictactoeGame*, int> winners; // Keeps track of winners corresponding to each game instance.
map<int,RESTART> ready_to_replay; // Used for deciding whether a game should be replayed or not.

void check(int result, const char* fail_msg, const char* success_msg);
void handle_connection(int clientfd, int player_num);
tictactoeGame* handle_partner_disconnect(tictactoe_packet* send_pkt, int p1, int p2);
void send_msg_disp(int clientfd, char *msg);
int play_game(tictactoeGame *game, int pid);

int main(){
    int nsocket;

    // initializing the socket desrciptor
    // SOCK_STREAM - TCP (2 way connection, reliable, and error free data is passed sequentially)
    fail_msg = "Socket open failed.";
    success_msg = "Socket opened successfully on port "+to_string(SERVER_PORT)+".";
    check((serverfd = socket(AF_INET, SOCK_STREAM, 0)), fail_msg.c_str(), success_msg.c_str());

    int optval = 1;
    int optlen = sizeof(optval);
    fail_msg = "Setting socket option failed.";
    success_msg = "Socket permits reuse of address.";
    check((setsockopt(serverfd, SOL_SOCKET ,SO_REUSEADDR, (const char*)&optval , optlen)), fail_msg.c_str(), success_msg.c_str());

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the newly created socket to an IP address and port.
    fail_msg = "Failed to bind to local port!";
    success_msg = "Socket bound to port "+to_string(SERVER_PORT)+" at IP address .";
    check(bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr)), fail_msg.c_str(), success_msg.c_str());

    // Listen for connections.
    // Maximum size of queue is 1.
    fail_msg = "Failed to listen for connections.";
    success_msg = "Server is listening for connections.";
    check(listen(serverfd, SERVER_BACKLOG), fail_msg.c_str(), success_msg.c_str());

    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);
    int nlen = sizeof(struct sockaddr);

    logger.LogInfo("Game server started. Waiting for players.....");

    // Server socket handles multiple clients simultaneously forking a
    // child process whenever a request from the client occurs. This child
    // process will connect to the client acting as a server while one
    // main listening socket always handles other clients.
    array<unique_ptr<thread>, 100> t_ptr_arr;
    int curr = 0;
    while(true){
        fail_msg = "Error on accepting the new connection request.";
        success_msg = "Connection Established with client.";
        check((clientfd = accept(serverfd, (struct sockaddr*)&client_addr, (socklen_t*)&nlen)), fail_msg.c_str(), success_msg.c_str());
        cout<<"Connection from ip "<<inet_ntoa(client_addr.sin_addr)<<" at port "<<ntohs(client_addr.sin_port)<<endl;
        string ip(inet_ntoa(client_addr.sin_addr));
        log_msg = "Connection from ip "+ip+" at port "+to_string(ntohs(client_addr.sin_port));
        logger.LogInfo(log_msg);
        ++curr_num_of_connections;
        while(t_ptr_arr[curr%100]!=NULL){
            ++curr;
        }
        t_ptr_arr[curr] = make_unique<thread>(handle_connection, clientfd, curr_num_of_connections%2);
    }
    close(serverfd);
    return 0;
}

void send_msg_disp(int clientfd, char *msg){
    send(clientfd, msg, strlen(msg), 0);
}

array<int, 2> sanitize_input(struct tictactoe_packet* send_pkt, int p, int opp){
    send(p, send_pkt, sizeof(send_pkt), 0);
    array<int, 2> rc = {-1,-1};
    struct tictactoe_packet recv_player_packet;
    int nbytesrecved = recv(p, &recv_player_packet, sizeof(recv_player_packet), 0);
    while(nbytesrecved<0 || nbytesrecved == 0 || recv_player_packet.msg_type == REPLAY_GAME ||
        recv_player_packet.row<1 || recv_player_packet.row>3 ||
        recv_player_packet.col<1 || recv_player_packet.col>3 ||
        games[p]->game_matrix[recv_player_packet.row-1][recv_player_packet.col-1]!=EMPTY){
            if(recv_player_packet.msg_type==REPLAY_GAME){
                return rc;
            }
            if(!nbytesrecved){
                rc[0] = -2;
                return rc;
            }
            send(p, send_pkt, sizeof(send_pkt), 0);
            nbytesrecved = recv(p, &recv_player_packet, sizeof(recv_player_packet), 0);
    }

    // Updating the matrix of both players.
    recv_player_packet.msg_type = OPPONENT_MOVE;
    send(opp, &recv_player_packet, sizeof(recv_player_packet), 0);
    recv_player_packet.msg_type = MOVE_SUCCESS;
    send(p, &recv_player_packet, sizeof(recv_player_packet), 0);

    // Logging player moves.
    log_msg = "Game "+to_string(games[p]->game_id)+": Player "+to_string(p)+" move: "+to_string(recv_player_packet.row)+" "+to_string(recv_player_packet.col);
    logger.LogInfo(log_msg);

    rc[0] = recv_player_packet.row-1;
    rc[1] = recv_player_packet.col-1;
    return rc;
}

int play_game(int pid, int p1, int p2){
    int nbytesrecved;
    struct tictactoe_packet send_pkt;
    memset(&send_pkt, 0, sizeof(send_pkt));
    int player_to_move;
    array<int, 2> rc;
    if(pid==1){
        memset(games[p1]->game_matrix, 0, sizeof(games[p1]->game_matrix));
        send_pkt.msg_type = SERVER_REQUEST_MOVE;
        rc = sanitize_input(&send_pkt, p1, p2);
        games[p1]->update_matrix(rc[0], rc[1], CIRCLE);
        // if(rc[0]==-1){
        //     winners[games[p1]] = TIMEOUT;
        //     games[p1]->is_p1_move = false;
        //     return TIMEOUT;
        // }else if(rc[0]==-2){
        //     winners[games[p1]] = DISCONNECT;
        //     games[p1]->is_p1_move = false;
        //     cout<<"hello"<<endl;
        //     return DISCONNECT;
        // }
        games[p1]->is_p1_move = false;
    }
    while(games[p1]->is_p1_move);

    // The player making the next move.
    int player, opp;
    winners[games[p1]] = -1;
    while(winners[games[p1]]<0){
        games[p1]->m.lock();
        if(winners[games[p1]]>=0){
            games[p1]->m.unlock();
            return winners[games[p1]];
        }
        if(games[p1]->is_p1_move){
            player = p1;
            opp = p2;
        }else{
            player= p2;
            opp = p1;
        }
        send_pkt.msg_type = SERVER_REQUEST_MOVE;
        rc = sanitize_input(&send_pkt, player, opp);
        if(rc[0]==-1){
            winners[games[p1]] = TIMEOUT;
            games[p1]->m.unlock();
            return winners[games[p1]];
        }else if(rc[0]==-2){
            winners[games[p1]] = DISCONNECT;
            games[p1]->m.unlock();
            return winners[games[p1]];
        }
        symbol curr_sym;
        (player==p1)?curr_sym = CIRCLE:curr_sym = CROSS;
        games[player]->update_matrix(rc[0], rc[1], curr_sym);
        (player==p1)?games[p1]->is_p1_move = false:games[p1]->is_p1_move = true; 
        winners[games[p1]] = games[p1]->check_winner();
        games[p1]->m.unlock();
    }
    return winners[games[p1]];
}

// Handles different clients on separate threads.
// Notifies players of their symbols and ids.
// Gives green light for game start once a pair is formed.
void handle_connection(int clientfd, int player_num){
    char *welcome_msg;
    unique_ptr<tictactoeGame> game;
    int p1=0,p2=0;
    int winner;

    // Flags to prevent logging of disconnection or timeout multiple times.
    int timeout_flag = 1, disconnect_flag = 1;
    if(player_num==1){
        welcome_msg = "Connected to the game server. Your player ID is 1. Waiting for a partner to join . . .";
        id_player_waiting = clientfd;
        send_msg_disp(clientfd, welcome_msg);

        // Waiting for opponent to join.
        while(!partners[clientfd]);
        p1 = id_player_waiting;
        id_player_waiting = NOONE;
        p2 = partners[clientfd];
        welcome_msg = "Your partner's ID is 2. Your symbol is 'O'.\n";
        send_msg_disp(clientfd, welcome_msg);
        
        while(games[p1]==NULL);
        while(!games[p1]->is_p1_move);
    }
    else{
        welcome_msg = "Connected to the game server. Your player ID is 2. Your partner's ID is 1. Your symbol is 'X'";
        send_msg_disp(clientfd, welcome_msg);

        // Assigning a game to both new players.
        // Storing the player pair and game id in a map "partners"        
        game = make_unique<tictactoeGame>(id_player_waiting, CIRCLE, clientfd, CROSS);

        p1 = id_player_waiting;
        p2 = clientfd;
        partners[p1] = p2;
        partners[p2] = p1;
        games[p1] = game.get();
        games[p2] = game.get();
        while(games[p1]->is_p1_move);
    }
    if(player_num==1){
        log_msg = "Players "+to_string(p1)+" and "+to_string(p2)+" have been paired.\nInitialising game with game id "+
            to_string(games[p1]->game_id)+".\nAllotting symbol 'O' to "+to_string(p1)+" and 'X' to "+to_string(p2);
        logger.LogInfo(log_msg);
    }
    ready_to_replay[p1] = YES;
    ready_to_replay[p2] = YES;
    while(ready_to_replay[p1]==YES && ready_to_replay[p2]==YES){

        // Starting the game.
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        winner = play_game(player_num, p1, p2);
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        // Game time calculation.
        if(player_num==1){
            long double total_time_sec = 0;
            total_time_sec = ((long double)(end_time.tv_nsec-start_time.tv_nsec))/1000000000.0;
            total_time_sec = total_time_sec + (end_time.tv_sec-start_time.tv_sec);
            log_msg = "Game "+to_string(games[p1]->game_id)+" lasted for "+to_string(total_time_sec)+" seconds.";
            logger.LogInfo(log_msg);
        }

        // Notifying the clients about the results.
        struct tictactoe_packet send_pkt, recv_pkt;
        switch(winner){
            case CIRCLE:
                log_msg = "Game "+to_string(games[p1]->game_id)+" won by Player "+to_string(p1)+".";
                if(player_num==1){
                    send_pkt.msg_type = WIN;
                }else{
                    send_pkt.msg_type = LOSS;
                }
                break;
            case CROSS:
                log_msg = "Game "+to_string(games[p1]->game_id)+" won by Player "+to_string(p2)+".";
                if(player_num==1){
                    send_pkt.msg_type = LOSS;
                }else{
                    send_pkt.msg_type = WIN;
                }
                break;
            case EMPTY:
                log_msg = "Game "+to_string(games[p1]->game_id)+" drawn.";
                send_pkt.msg_type = DRAW;
                break;
            case TIMEOUT:
                log_msg = "Game "+to_string(games[p1]->game_id)+" discontinued due to player not responding.";
                if(player_num==1){
                    logger.LogInfo(log_msg);
                }
                goto rep;
            case DISCONNECT:
                if(player_num==1){
                    log_msg = "Game "+to_string(games[p1]->game_id)+": Player disconnected.";
                    logger.LogInfo(log_msg);
                }
                send_pkt.msg_type = PARTNER_DISCONNECT;
                send(clientfd, &send_pkt, sizeof(send_pkt), 0);
                goto end;
        }
        send(clientfd, &send_pkt, sizeof(send_pkt), 0);

    rep:
        ready_to_replay[p1] = NO_RESPONSE;
        ready_to_replay[p2] = NO_RESPONSE;

        // Server asking if clients want to play again.
        send_pkt.msg_type = REPLAY;
        send(clientfd, &send_pkt, sizeof(send_pkt), 0);

        // Clearing the matrix for a possible restart.
        memset(games[p1]->game_matrix, EMPTY, sizeof(games[p1]->game_matrix));

        int opp = partners[clientfd];
        int nbytesrecved = recv(clientfd, &recv_pkt, sizeof(recv_pkt), 0);
        if(recv_pkt.msg_type == END_GAME){
            games[p1] = NULL;
            ready_to_replay[clientfd] = NO;
        }
        else if(recv_pkt.msg_type == RESTART_GAME){
            ready_to_replay[clientfd] = YES;
            while(ready_to_replay[opp]==NO_RESPONSE);
            if(ready_to_replay[partners[clientfd]]==YES){
                games[p1]->is_p1_move = true;
            }else{
                send_pkt.msg_type = PARTNER_DISCONNECT;
                send(clientfd, &send_pkt, sizeof(send_pkt), 0);
            }
        }
    }
end:
    partners[p1] = 0;
    partners[p2] = 0;
}

void check(int result, const char* fail_msg, const char* success_msg){
    if(result<0){
        logger.LogInfo(fail_msg);
        std::exit(EXIT_FAILURE);
    }
    logger.LogInfo(success_msg);
}