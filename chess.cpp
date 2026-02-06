#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

using std::cout;

constexpr char newline = '\n';
constexpr uint8_t typeMask = 7;
constexpr uint8_t colorMask = 24;

enum Piece : std::uint8_t {
    None = 0,
    Pawn = 1, Knight = 2, Bishop = 3, Rook = 4, Queen = 5, King = 6,
    White = 8,
    Black = 16
};

class ChessBoard {
private:
    std::array<std::array<std::uint8_t, 8>, 8> board;

    // Initialization helpers
    constexpr void setTop() {
        for (int col = 0; col < 8; col++) this->board[1][col] = Piece::Pawn | Piece::Black;
        this->board[0][0] = this->board[0][7] = Piece::Rook | Piece::Black;
        this->board[0][1] = this->board[0][6] = Piece::Bishop | Piece::Black;
        this->board[0][2] = this->board[0][5] = Piece::Knight | Piece::Black;
        this->board[0][3] = Piece::Queen | Piece::Black;
        this->board[0][4] = Piece::King | Piece::Black;
    }

    constexpr void setBottom() {
        for (int col = 0; col < 8; col++) this->board[6][col] = Piece::Pawn | Piece::White;
        this->board[7][0] = this->board[7][7] = Piece::Rook | Piece::White;
        this->board[7][1] = this->board[7][6] = Piece::Bishop | Piece::White;
        this->board[7][2] = this->board[7][5] = Piece::Knight | Piece::White;
        this->board[7][3] = Piece::Queen | Piece::White;
        this->board[7][4] = Piece::King | Piece::White;
    }

    constexpr void setMiddle() {
        for (int row = 2; row < 6; row++)
            for (int col = 0; col < 8; col++)
                this->board[row][col] = Piece::None;
    }

    // input validation / bounds check
    bool checkFormat(const std::string& pos, int& row, int& col) {
        if (pos.size() == 2 && (pos[0] >= 'a' && pos[0] <= 'h') && (pos[1] >= '1' && pos[1] <= '8')) {
            col = pos[0] - 'a';
            row = 8 - (pos[1] - '0');
            return true;
        }
        return false;
    }

    // piece logic check
    bool isLegalMove(int currR, int currC, int moveR, int moveC) const {
        uint8_t piece = board[currR][currC];
        if (piece == Piece::None) return false;

        uint8_t target = board[moveR][moveC];
        int rowDiff = moveR - currR;
        int colDiff = moveC - currC;
        int direction = (piece & Piece::White) ? -1 : 1;

        switch (piece & typeMask) {
        case Piece::Pawn:
            // Move forward 1
            if (colDiff == 0 && rowDiff == direction && target == Piece::None) return true;
            // Move forward 2 from start
            if (colDiff == 0 && rowDiff == 2 * direction) {
                int startRow = (piece & Piece::White) ? 6 : 1;
                if (currR == startRow && target == Piece::None) return true;
            }
            // Capture
            if (colDiff == 1 && rowDiff == direction && target != Piece::None) {
                if ((piece & colorMask) != (target & colorMask)) return true;
            }
            return false;
        case Piece::Rook:
            // move horizontal
            if (rowDiff == 0) {
                if (colDiff > 0) {
                    for (int col = currC + 1; currC < moveC; currC++) {
                        if (this->board[currR][col] != Piece::None) {
                            return false;
                        }
                    }
                }
                else {
                    for (int col = currC - 1; currC > moveC; currC--) {
                        if (this->board[currR][col] != Piece::None) {
                            return false;
                        }
                    }
                }
            }
            else if (colDiff == 0) {

            }

            // move vertical
            return true;
        case Piece::Bishop: 
            return false;
        case Piece::Knight:
            return false;
        case Piece::Queen:
            return false;
        case Piece::King:
            return false;
        }
    }

    // returns a string instead of immediatly printing so we can flush the buffer all at once
    std::string getPieceString(uint8_t piece) const {
        std::string s = "";
        char c = '.';
        switch (piece & typeMask) {
        case Piece::Pawn:   c = 'P'; break;
        case Piece::Rook:   c = 'R'; break;
        case Piece::Bishop: c = 'B'; break;
        case Piece::Knight: c = 'N'; break;
        case Piece::Queen:  c = 'Q'; break;
        case Piece::King:   c = 'K'; break;
        }

        if (piece & Piece::White) s = "\033[1;37m" + std::string(1, c) + "\033[0m ";
        else if (piece & Piece::Black) s = "\033[1;34m" + std::string(1, c) + "\033[0m ";
        else s = ". ";

        return s;
    }

public:
    ChessBoard() {
        setTop();
        setMiddle();
        setBottom();
    }

    void makeMove() {
        std::string pos1, pos2;
        int currR, currC, moveR, moveC;

        // piece selection
        cout << "\nPiece to move: ";
        while (!(std::cin >> pos1 && checkFormat(pos1, currR, currC))) {
            cout << "Invalid format. Try again: ";
        }

        cout << "\033[H\033[2J"; // Clear screen

        // generates a flickering effect for visual confirmation of selected piece
        std::atomic<bool> keepFlickering{ true };
        std::thread flickerThread([this, &keepFlickering, currR, currC]() {
            bool hide = false;
            while (keepFlickering) {
                cout << "\033[H"; // Cursor to top-left
                this->render(currR, currC, hide);
                hide = !hide;
                cout << "\nMove to: ";
                cout.flush();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
            }
            });

        // destination selection
        while (!(std::cin >> pos2 && checkFormat(pos2, moveR, moveC))) {
            cout << "Invalid format. Try again: ";
        }

        keepFlickering = false;
        if (flickerThread.joinable()) flickerThread.join();

        // check if we can make the move and execute
        if (isLegalMove(currR, currC, moveR, moveC)) {
            board[moveR][moveC] = board[currR][currC];
            board[currR][currC] = Piece::None;
            cout << "\033[H\033[2J"; // Clear screen
            render();
        }
        else {
            cout << "\nIllegal Move! Press Enter to continue...";
            std::cin.ignore(); std::cin.get();
        }
    }

    void render(int r = -1, int c = -1, bool hide = false) const {
        std::string output = "\033[H";

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (row == r && col == c && hide) {
                    output += "  ";
                }
                else {
                    output += getPieceString(this->board[row][col]);
                }
            }
            output += "\n";
        }

        // One single I/O operation!
        std::cout << output << std::flush;
    }
};

int main() {
    ChessBoard game;
    cout << "\033[2J"; // Initial clear
    game.render();
    while (true) {
        game.makeMove();
    }
    return 0;
}