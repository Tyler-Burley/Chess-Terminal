#include <array>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <vector>

#define DEBUG_MODE 1

using std::cout;

constexpr char newline = '\n';
constexpr uint8_t typeMask = 7;
constexpr uint8_t colorMask = 24;
constexpr uint8_t whiteMask = 8;
constexpr uint8_t blackMask = 16;


enum Piece : std::uint8_t {
    None = 0,
    Pawn = 1, Knight = 2, Bishop = 3, Rook = 4, Queen = 5, King = 6,
    White = 8,
    Black = 16
};

// Return types for game state checks
enum class GameState {
    Playing,
    Check,
    Checkmate,
    Stalemate
};

class ChessBoard {
private:
    std::array<std::array<std::uint8_t, 8>, 8> board;
    std::array<std::uint8_t, 7> whitesTakenPieces = { 0 };
    std::array<std::uint8_t, 7> blacksTakenPieces = { 0 };

    // Initialization
    void setTop() {
        for (int col = 0; col < 8; col++) this->board[1][col] = Piece::Pawn | Piece::Black;
        this->board[0][0] = this->board[0][7] = Piece::Rook | Piece::Black;
        this->board[0][1] = this->board[0][6] = Piece::Knight | Piece::Black;
        this->board[0][2] = this->board[0][5] = Piece::Bishop | Piece::Black;
        this->board[0][3] = Piece::Queen | Piece::Black;
        this->board[0][4] = Piece::King | Piece::Black;
    }

    void setBottom() {
        for (int col = 0; col < 8; col++) this->board[6][col] = Piece::Pawn | Piece::White;
        this->board[7][0] = this->board[7][7] = Piece::Rook | Piece::White;
        this->board[7][1] = this->board[7][6] = Piece::Knight | Piece::White;
        this->board[7][2] = this->board[7][5] = Piece::Bishop | Piece::White;
        this->board[7][3] = Piece::Queen | Piece::White;
        this->board[7][4] = Piece::King | Piece::White;
    }

    void setMiddle() {
        for (int row = 2; row < 6; row++)
            for (int col = 0; col < 8; col++)
                this->board[row][col] = Piece::None;
    }

    // --- Helpers ---

    bool checkFormat(const std::string& pos, int& row, int& col) const {
        if (pos.size() == 2 && (pos[0] >= 'a' && pos[0] <= 'h') && (pos[1] >= '1' && pos[1] <= '8')) {
            col = pos[0] - 'a';
            row = 8 - (pos[1] - '0');
            return true;
        }
        return false;
    }

    bool checkColor(bool isWhitesTurn, uint8_t piece) const {
        return isWhitesTurn == ((piece & colorMask) == whiteMask);
    }

    // Locates the King of a specific color
    void findKing(uint8_t color, int& r, int& c) const {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if ((board[i][j] & typeMask) == Piece::King && (board[i][j] & colorMask) == color) {
                    r = i; c = j; return;
                }
            }
        }
        r = -1; c = -1; // Should never happen
    }

    // CORE LOGIC: Returns true if the square (r, c) is being attacked by 'attackerColor'
    bool isSquareAttacked(int r, int c, uint8_t attackerColor) const {
        // 1. Knight Attacks
        int knightMoves[8][2] = { {-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1} };
        for (auto& move : knightMoves) {
            int nr = r + move[0];
            int nc = c + move[1];
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                uint8_t p = board[nr][nc];
                if ((p & typeMask) == Piece::Knight && (p & colorMask) == attackerColor) return true;
            }
        }

        // 2. Sliding (Rook/Queen)
        int straightDirs[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
        for (auto& dir : straightDirs) {
            for (int dist = 1; dist < 8; dist++) {
                int nr = r + (dir[0] * dist);
                int nc = c + (dir[1] * dist);
                if (nr < 0 || nr >= 8 || nc < 0 || nc >= 8) break;
                uint8_t p = board[nr][nc];
                if (p != Piece::None) {
                    if ((p & colorMask) == attackerColor &&
                        ((p & typeMask) == Piece::Rook || (p & typeMask) == Piece::Queen)) return true;
                    break;
                }
            }
        }

        // 3. Sliding (Bishop/Queen)
        int diagDirs[4][2] = { {-1,-1}, {-1,1}, {1,-1}, {1,1} };
        for (auto& dir : diagDirs) {
            for (int dist = 1; dist < 8; dist++) {
                int nr = r + (dir[0] * dist);
                int nc = c + (dir[1] * dist);
                if (nr < 0 || nr >= 8 || nc < 0 || nc >= 8) break;
                uint8_t p = board[nr][nc];
                if (p != Piece::None) {
                    if ((p & colorMask) == attackerColor &&
                        ((p & typeMask) == Piece::Bishop || (p & typeMask) == Piece::Queen)) return true;
                    break;
                }
            }
        }

        // 4. Pawn Attacks
        int pawnRowDir = (attackerColor == Piece::White) ? 1 : -1; // Invert check direction
        int pawnAttacks[2][2] = { {pawnRowDir, -1}, {pawnRowDir, 1} };
        for (auto& attack : pawnAttacks) {
            int nr = r + attack[0];
            int nc = c + attack[1];
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                uint8_t p = board[nr][nc];
                if ((p & typeMask) == Piece::Pawn && (p & colorMask) == attackerColor) return true;
            }
        }

        // 5. King Attacks
        int kingMoves[8][2] = { {-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1} };
        for (auto& move : kingMoves) {
            int nr = r + move[0];
            int nc = c + move[1];
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                uint8_t p = board[nr][nc];
                if ((p & typeMask) == Piece::King && (p & colorMask) == attackerColor) return true;
            }
        }

        return false;
    }

    // Checks "Geometry" only (how pieces move, ignoring checks)
    bool validateGeometry(int currR, int currC, int moveR, int moveC) const {
        uint8_t piece = board[currR][currC];
        if (piece == Piece::None) return false;
        uint8_t target = board[moveR][moveC];

        // Friendly fire check
        if (target != Piece::None) {
            if ((piece & colorMask) == (target & colorMask)) return false;
        }

        int rowDiff = moveR - currR;
        int colDiff = moveC - currC;
        int absRow = std::abs(rowDiff);
        int absCol = std::abs(colDiff);

        switch (piece & typeMask) {
        case Piece::Pawn: {
            int direction = (piece & Piece::White) ? -1 : 1;
            // Move 1
            if (colDiff == 0 && rowDiff == direction) return target == Piece::None;
            // Move 2
            if (colDiff == 0 && rowDiff == 2 * direction) {
                int startRow = (piece & Piece::White) ? 6 : 1;
                if (currR == startRow && target == Piece::None && board[currR + direction][currC] == Piece::None) return true;
            }
            // Capture
            if (absCol == 1 && rowDiff == direction) return target != Piece::None;
            return false;
        }
        case Piece::Knight:
            return (absRow == 2 && absCol == 1) || (absRow == 1 && absCol == 2);
        case Piece::King:
            return (absRow <= 1 && absCol <= 1);
        case Piece::Rook:
        case Piece::Bishop:
        case Piece::Queen: {
            bool isStraight = (currR == moveR || currC == moveC);
            bool isDiagonal = (absRow == absCol);
            uint8_t type = piece & typeMask;
            if (type == Piece::Rook && !isStraight) return false;
            if (type == Piece::Bishop && !isDiagonal) return false;
            if (type == Piece::Queen && !isStraight && !isDiagonal) return false;

            int rowStep = (rowDiff == 0) ? 0 : (rowDiff > 0 ? 1 : -1);
            int colStep = (colDiff == 0) ? 0 : (colDiff > 0 ? 1 : -1);
            int steps = std::max(absRow, absCol);
            for (int i = 1; i < steps; i++) {
                if (board[currR + (i * rowStep)][currC + (i * colStep)] != Piece::None) return false;
            }
            return true;
        }
        default: return false;
        }
    }

    bool isSafeMove(int currR, int currC, int moveR, int moveC) {

        // 1. Check Geometry
        if (!validateGeometry(currR, currC, moveR, moveC)) return false;

        // 2. Simulate Move
        uint8_t originalSource = board[currR][currC];
        uint8_t originalDest = board[moveR][moveC];

        board[moveR][moveC] = originalSource;
        board[currR][currC] = Piece::None;

        // 3. Am I in Check?
        uint8_t myColor = originalSource & colorMask;
        uint8_t enemyColor = (myColor == Piece::White) ? Piece::Black : Piece::White;

        int kR, kC;
        findKing(myColor, kR, kC);
        bool inCheck = isSquareAttacked(kR, kC, enemyColor);

        // 4. Undo Move
        board[currR][currC] = originalSource;
        board[moveR][moveC] = originalDest;

        return !inCheck;
    }

    // Loops through ALL pieces to see if ANY valid move exists
    bool hasAnyLegalMoves(uint8_t color) {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if ((board[r][c] & colorMask) == color) {
                    // Try moving this piece to every square
                    for (int tr = 0; tr < 8; tr++) {
                        for (int tc = 0; tc < 8; tc++) {
                            // This calls validateGeometry AND the safety check
                            if (isSafeMove(r, c, tr, tc)) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    std::string getPieceString(uint8_t piece, bool highlight = false) const {
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

        std::string colorCode = "";
        if (piece & Piece::White) colorCode = "\033[1;37m";
        else if (piece & Piece::Black) colorCode = "\033[1;34m";
        else colorCode = "\033[90m";

        if (highlight) s = "\033[7m" + colorCode + c + "\033[0m ";
        else s = colorCode + c + "\033[0m ";
        return s;
    }

    std::string getCapturedListString(const std::array<std::uint8_t, 7>& captureList, uint8_t pieceColor) const {
        std::string s = "";
        for (int type = 1; type <= 5; type++) {
            int count = captureList[type];
            if (count > 0) {
                uint8_t pieceToRender = type | pieceColor;
                for (int i = 0; i < count; i++) s += getPieceString(pieceToRender) + " ";
            }
        }
        return s;
    }

public:
    ChessBoard() {
        setTop();
        setMiddle();
        setBottom();
    }

    // Returns the state of the opponent (Check, Mate, etc.)
    GameState getGameState(uint8_t playerColor) {
        int kR, kC;
        uint8_t enemyColor = (playerColor == Piece::White) ? Piece::Black : Piece::White;
        findKing(playerColor, kR, kC);

        bool inCheck = isSquareAttacked(kR, kC, enemyColor);
        bool hasMoves = hasAnyLegalMoves(playerColor);

        if (inCheck && !hasMoves) return GameState::Checkmate;
        if (inCheck) return GameState::Check;
        if (!hasMoves) return GameState::Stalemate;
        return GameState::Playing;
    }

    void makeMove(bool isWhitesTurn, bool& isRunning) {
        std::string pos1, pos2;
        int currR, currC, moveR, moveC;

        while (true) {
            cout << "\nPiece to move " << ((isWhitesTurn) ? "(white)" : "(black)") << " : ";

            while (true) {
                std::cin >> pos1;
                if (checkFormat(pos1, currR, currC) && checkColor(isWhitesTurn, this->board[currR][currC])) {
                    break;
                }
                cout << "Invalid selection. Try again: ";
            }

            cout << "\033[H\033[2J";

            // FLICKER THREAD
            std::atomic<bool> keepFlickering{ true };
            std::thread flickerThread([this, &keepFlickering, currR, currC]() {
                bool highlight = false;
                while (keepFlickering) {
                    cout << "\033[H";
                    this->render(currR, currC, highlight);
                    highlight = !highlight;
                    cout << "\nSelected: " << (char)('a' + currC) << 8 - currR << "\n";
                    cout << "Move to (type 'x' to cancel): ";
                    cout.flush();
                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                }
                });

            std::cin >> pos2;
            keepFlickering = false;
            if (flickerThread.joinable()) flickerThread.join();

            if (pos2 == "x" || pos2 == "X") {
                cout << "\033[H\033[2J";
                render();
                continue;
            }

            if (!checkFormat(pos2, moveR, moveC)) {
                cout << "\033[H\033[2J";
                render();
                cout << "\nInvalid format!\n";
                continue;
            }

            if (isSafeMove(currR, currC, moveR, moveC)) {

                // Capture Logic
                uint8_t target = this->board[moveR][moveC];
                if (target != Piece::None) {
                    if (isWhitesTurn) this->whitesTakenPieces[target & typeMask]++;
                    else this->blacksTakenPieces[target & typeMask]++;
                }

                // Commit Move
                board[moveR][moveC] = board[currR][currC];
                board[currR][currC] = Piece::None;

                cout << "\033[H\033[2J";
                render();

                // --- END OF TURN CHECK ---
                uint8_t opponentColor = isWhitesTurn ? Piece::Black : Piece::White;
                GameState state = getGameState(opponentColor);

                if (state == GameState::Checkmate) {
                    cout << "\nCHECKMATE! " << (isWhitesTurn ? "White" : "Black") << " wins!\n";
                    isRunning = false;
                }
                else if (state == GameState::Stalemate) {
                    cout << "\nSTALEMATE! It's a draw.\n";
                    isRunning = false;
                }
                else if (state == GameState::Check) {
                    cout << "\nCHECK!\n";
                }

                return;
            }
            else {
                cout << "\033[H\033[2J";
                render();
                // This message now covers self-check too
                cout << "\nIllegal Move! (Rule violation or King is in check)\n";
            }
        }
    }

    // Renders both perspectives side-by-side
    void render(int selectedR = -1, int selectedC = -1, bool highlight = false) const {
        std::string output = "\033[H"; // Home cursor

        // 1. Headers
        output += "         WHITE PERSPECTIVE                          BLACK PERSPECTIVE\n";
        output += "\n";

        for (int i = 0; i < 8; i++) {

            // --- LEFT BOARD  ---
            int wRow = i;
            output += "\033[90m" + std::to_string(8 - wRow) + "   \033[0m";

            for (int col = 0; col < 8; col++) {
                bool isSelected = (wRow == selectedR && col == selectedC && highlight);
                output += getPieceString(this->board[wRow][col], isSelected) + "  ";
            }

            output += "       ";

            // --- RIGHT BOARD ---
            int bRow = 7 - i;

            output += "\033[90m" + std::to_string(8 - bRow) + "   \033[0m";
            for (int col = 7; col >= 0; col--) {
                bool isSelected = (bRow == selectedR && col == selectedC && highlight);
                output += getPieceString(this->board[bRow][col], isSelected) + "  ";
            }

            // --- FAR RIGHT ---
            output += "   ";
            if (i == 0) output += "Taken by White: " + getCapturedListString(whitesTakenPieces, Piece::Black);
            if (i == 1) output += "Taken by Black: " + getCapturedListString(blacksTakenPieces, Piece::White);

            output += "\n\n";
        }

        // --- BOTTOM LABELS ---
        // Left: a..h
        output += "\033[90m";
        output += "\n    a   b   c   d   e   f   g   h";

        // Right: h..a
        output += "          ";
        output += "    h   g   f   e   d   c   b   a\n";
        output += "\033[0m";

        std::cout << output << std::flush;
    }
};

int main() {
    ChessBoard game;
    bool isRunning = true;
    bool isWhitesTurn = true;
    cout << "\033[2J";
    game.render();
    while (isRunning) {
        game.makeMove(isWhitesTurn, isRunning);
        isWhitesTurn = !isWhitesTurn;
    }
    return 0;
}