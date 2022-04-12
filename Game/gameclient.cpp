#include <iostream>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <unistd.h> // For read
#include <cstring>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <packet.h>

#define PORT 8911
#define MAX_BUFFER_SIZE 1024

using namespace std;

char game_matrix[3][3];
int clientfd;
bool p1 = false;
struct sockaddr_in server_addr;
struct tictactoe_packet move_pkt;

int input_available();
void check(int result, const char* msg);
void fill_matrix();
void game_matrix_print();
void update_game_matrix(int r, int c, bool player1, string move_maker);
int recv_msg_disp(int cfd);
void start_game(bool p);

int main(int argc, char** argv){
    if(argc != 2){
        cout<<"Usage: ./gclient {ip address of server}"<<endl;
        std::exit(EXIT_FAILURE);
    }

    check((clientfd = socket(AF_INET, SOCK_STREAM, 0)), "SOCKET OPEN FAILED\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Checking if IP is valid and including server ip in server_addr structure.
    if(inet_aton(argv[1], &server_addr.sin_addr)==0){
        cout<<"Bad Hostname.\n";
        std::exit(EXIT_FAILURE);
    }
    
    // Establishing connection with game server.
    check(connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)), "Connection Failed\n");
    
    int nbytesreceived = recv_msg_disp(clientfd);
    if(nbytesreceived==86){
        recv_msg_disp(clientfd);
        p1 = true;
    }
    start_game(p1);
	return 0;	
}

int input_available(){
    // Watch stdin "clientfd" to see when it has input.
    fd_set fr;
    FD_ZERO(&fr);
    FD_SET(0, &fr);

    // Set timeout to 15 seconds.
    struct timeval tv;
    tv.tv_sec = 15;
    tv.tv_usec = 0;

    int retval, r, c;
    retval = select(1, &fr, NULL, NULL, &tv);
    return retval;
}

void check(int result, const char* msg){
    if(result<0){
        cout<<msg;
        std::exit(EXIT_FAILURE);
    }
}

void fill_matrix(){
    for(int i=0;i<3;++i){
        for(int j=0;j<3;++j){
            game_matrix[i][j] = '_';
        }
    }
}

void game_matrix_print(){
    for(int i=0;i<3;++i){
        for(int j=0;j<2;++j){
            cout<<game_matrix[i][j]<<" | ";
        }
        cout<<game_matrix[i][2]<<endl;
    }
}

void update_game_matrix(int r, int c, bool player1, string move_maker){
    if(move_maker!="OPP"){
        player1?game_matrix[r-1][c-1] = 'O':game_matrix[r-1][c-1] = 'X';
    }else{
        player1?game_matrix[r-1][c-1] = 'X':game_matrix[r-1][c-1] = 'O';
    }
}

int recv_msg_disp(int cfd){
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    int nbytesrecved = 0;
    check((nbytesrecved = recv(cfd, &buffer, MAX_BUFFER_SIZE, 0)), "Error receiving message");
    cout<<buffer<<"\n";
    return nbytesrecved;
}

void start_game(bool player1){
    fill_matrix();
    cout<<"Starting the game....\n";
    game_matrix_print();

    memset(&move_pkt, 0, sizeof(move_pkt));
    int r,c;
    int replay = 0;
    string response;
    while(true){
        int nbytesrecved = 0;
        do{
            check((nbytesrecved = recv(clientfd, &move_pkt, sizeof(move_pkt), 0)), "Error receiving message");
        }while(!nbytesrecved);
        switch(move_pkt.msg_type){
            case SERVER_REQUEST_MOVE:
                if(replay){
                    game_matrix_print();
                    replay = 0;
                }
                cout<<"Enter (ROW, COL) for placing your mark: "<<endl;
                if(input_available()){
                    cin>>r>>c;
                    move_pkt.msg_type = PLAYER_MOVE;
                    move_pkt.row = r;
                    move_pkt.col = c;
                }else{
                    move_pkt.msg_type = REPLAY_GAME;
                    move_pkt.row = -1;
                    move_pkt.col = -1;
                }
                send(clientfd, &move_pkt, sizeof(move_pkt),0);
                break;
            case OPPONENT_MOVE:
                update_game_matrix(move_pkt.row, move_pkt.col, player1, "OPP");
                game_matrix_print();
                break;
            case MOVE_SUCCESS:
                update_game_matrix(move_pkt.row, move_pkt.col, player1, "OWN");
                game_matrix_print();
                cout<<"Move made succesfully!"<<endl;
                break;
            case WIN:
                cout<<"Congratulions! You are the winner."<<endl;
                break;
            case LOSS:
                cout<<"Better luck next time."<<endl;
                break;
            case DRAW:
                cout<<"Game tied!"<<endl;
                break;
            case REPLAY:
                cout<<"Do you wish to replay the game? "<<endl;
                cin>>response;
                if(response[0]=='Y' || response[0]=='y'){
                    move_pkt.msg_type = RESTART_GAME;
                    replay = 1;
                    fill_matrix();
                }else{
                    move_pkt.msg_type = END_GAME;
                    send(clientfd, &move_pkt, sizeof(move_pkt),0);
                    return;
                }
                send(clientfd, &move_pkt, sizeof(move_pkt),0);
                break;
            case PARTNER_DISCONNECT:
                cout<<"Sorry, your partner disconnected."<<endl;
                cout<<"Please restart to arrange another game."<<endl;
                return;
        }
    }
}