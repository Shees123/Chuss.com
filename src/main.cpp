#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <sdl2/SDL_ttf.h>
#include <iostream>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <vector>
#include <cassert>

bool isAtLatestState = true;
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 640;
const int BOARD_WIDTH = 640;
const int BOARD_HEIGHT = 640;
const int BOARD_SIZE = 8;
const int TOTAL_SQUARES = BOARD_SIZE * BOARD_SIZE;
int squareSize = BOARD_WIDTH / BOARD_SIZE;
int boardX = (WINDOW_WIDTH - BOARD_WIDTH) / 2;
int boardY = 0;


class BoardStateList {
private:
    struct Node {
        std::string fen;  // FEN string representing the board state
        Node* prev;       // Pointer to the previous board state
        Node* next;       // Pointer to the next board state

        Node(const std::string& state) : fen(state), prev(nullptr), next(nullptr) {}
    };

    Node* head;    // Pointer to the head of the list (initial state)
    Node* tail;    // Pointer to the tail (most recent state)
    Node* current; // Pointer to the current state (used for undo/redo)

public:
    BoardStateList() : head(nullptr), tail(nullptr), current(nullptr){}

    // Add a new state to the list
    void AddState(const std::string& state) {
        Node* newNode = new Node(state);

        if (!head) {  // If the list is empty
            head = tail = current = newNode;
        } else {
            // Remove all forward states (clear redo) when adding new state
            if (current && current->next) {
                Node* temp = current->next;
                while (temp) {
                    Node* toDelete = temp;
                    temp = temp->next;
                    delete toDelete;
                }
                current->next = nullptr;
                tail = current;
            }

            // Add the new state
            current->next = newNode;
            newNode->prev = current;
            tail = newNode;
            current = tail;
        }
    }

    // Undo (move to the previous state)
    bool Undo(std::string& state) {
        isAtLatestState = false;
        std::cout<<"is latest state: "<<isAtLatestState<<std::endl;
        if (current && current->prev) {
            current = current->prev;  // Move to the previous state
            state = current->fen;
            return true;
        }
        return false;  // No previous state
    }

    // Redo (move to the next state)
    bool Redo(std::string& state) {
        if(tail->prev->fen == current->fen){
            isAtLatestState = true;
            std::cout<<"is latest state: "<<isAtLatestState<<std::endl;
        }
        if (current && current->next) {
            current = current->next;  // Move to the next state
            state = current->fen;
            return true;
        }
        return false;  // No next state
    }

    // Get the current state
    std::string GetCurrentState() const {
        return (current) ? current->fen : "";
    }

    // Clear the list (reset the game)
    void Clear() {
        Node* temp = head;
        while (temp) {
            Node* toDelete = temp;
            temp = temp->next;
            delete toDelete;
        }
        head = tail = current = nullptr;
    }

    ~BoardStateList() {
        Clear();
    }
};

class Piece {
public:
    std::vector<int> validMoves;
    const int none = 0;
    const int king = 1;
    const int pawn = 2;
    const int knight = 3;
    const int bishop = 4;
    const int rook = 5;
    const int queen = 6;

    const int white = 8;
    const int black = 16;
};

class Board {
public:
    int squares[64];
    Piece p;
    int currentTurn; // Tracks current player: p.white or p.black

    Board() {
        currentTurn = p.white; // White moves first
        const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        LoadPositionFromFen(startFen);
    }

    void LoadPositionFromFen(const std::string& fen) {
        std::unordered_map<char, int> pieceTypeFromSymbol = {
            {'k', p.king},   {'p', p.pawn}, {'n', p.knight},
            {'b', p.bishop}, {'r', p.rook}, {'q', p.queen}
        };

        std::string fenBoard = fen.substr(0, fen.find(' ')); // Extract board part
        int column = 0, row = 0;

        for (int i = 0; i < 64; i++) {
            squares[i] = p.none;
        }

        for (char symbol : fenBoard) {
            if (symbol == '/') {
                column = 0;
                row++;
            } else {
                if (isdigit(symbol)) {
                    column += symbol - '0';
                } else {
                    int pieceColour = isupper(symbol) ? p.white : p.black;
                    int pieceType = pieceTypeFromSymbol[tolower(symbol)];
                    squares[row * BOARD_SIZE + column] = pieceType | pieceColour;
                    column++;
                }
            }
        }
    }

    std::string GetFenFromPosition() {
    std::string fen = "";
    int emptyCount = 0;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            int square = squares[row * BOARD_SIZE + col];
            if (square == p.none) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }

                char pieceChar = ' ';
                int pieceType = square & 7; // Extract type ignoring color
                int pieceColor = square & (p.white | p.black); // Extract color

                if (pieceType == p.king)       pieceChar = 'k';
                else if (pieceType == p.queen) pieceChar = 'q';
                else if (pieceType == p.rook)  pieceChar = 'r';
                else if (pieceType == p.bishop) pieceChar = 'b';
                else if (pieceType == p.knight) pieceChar = 'n';
                else if (pieceType == p.pawn)  pieceChar = 'p';

                if (pieceColor == p.white) pieceChar = toupper(pieceChar);
                fen += pieceChar;
            }
        }

        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
            emptyCount = 0;
        }

        if (row != BOARD_SIZE - 1) fen += '/';
    }

    // Append default FEN fields (turn, castling rights, etc.)
    fen += " w - - 0 1"; 

    return fen;
}

void SwitchTurn() {
        currentTurn = (currentTurn == p.white) ? p.black : p.white;
    }

bool IsValidMove(int piece, int from, int to) {
    if (from == to) return false; // Can't move to the same square

    int fromRow = from / BOARD_SIZE;
    int fromCol = from % BOARD_SIZE;
    int toRow = to / BOARD_SIZE;
    int toCol = to % BOARD_SIZE;

    int deltaRow = toRow - fromRow;
    int deltaCol = toCol - fromCol;

    int pieceType = piece & 7; // Mask out color bits
    int pieceColor = piece & (p.white | p.black);

    // Prevent capturing your own piece
    if (squares[to] != p.none && (squares[to] & (p.white | p.black)) == pieceColor) {
        return false;
    }

    if (pieceType == p.king) {
    int opponentColor = (pieceColor == p.white) ? p.black : p.white;
    // Check if the target square is attacked by any opponent's piece
    for (int i = 0; i < 64; ++i) {
        if ((squares[i] & (p.white | p.black)) == opponentColor) {
            if (IsValidMove(squares[i], i, to)) {
                return false; // Target square is attacked
            }
        }
    }
}

    switch (pieceType) {
        case 2: // Pawn
            if (pieceColor == p.white) {
                if (deltaRow == -1 && deltaCol == 0 && squares[to] == p.none) return true; // Move forward
                if (fromRow == 6 && deltaRow == -2 && deltaCol == 0 && squares[to] == p.none) return true; // Double move
                if (deltaRow == -1 && abs(deltaCol) == 1 && squares[to] & p.black) return true; // Capture
                // En Passant
                if (deltaRow == -1 && abs(deltaCol) == 1 && squares[to] == p.none) {
                    int capturedPawn = (to + BOARD_SIZE) % 64;
                    if ((squares[capturedPawn] & 7) == 2 && (squares[capturedPawn] & (p.white | p.black)) == p.black) {
                        return true;
                    }
                }
            } else { // Black pawn
                if (deltaRow == 1 && deltaCol == 0 && squares[to] == p.none) return true; // Move forward
                if (fromRow == 1 && deltaRow == 2 && deltaCol == 0 && squares[to] == p.none) return true; // Double move
                if (deltaRow == 1 && abs(deltaCol) == 1 && squares[to] & p.white) return true; // Capture
                // En Passant
                if (deltaRow == 1 && abs(deltaCol) == 1 && squares[to] == p.none) {
                    int capturedPawn = (to - BOARD_SIZE) % 64;
                    if ((squares[capturedPawn] & 7) == 2 && (squares[capturedPawn] & (p.white | p.black)) == p.white) {
                        return true;
                    }
                }
            }
            return false;

        case 3: // Knight
            return (abs(deltaRow) == 2 && abs(deltaCol) == 1) || (abs(deltaRow) == 1 && abs(deltaCol) == 2);

        case 4: // Bishop
            if (abs(deltaRow) == abs(deltaCol)) {
                return IsPathClear(fromRow, fromCol, toRow, toCol);
            }
            return false;

        case 5: // Rook
            if (deltaRow == 0 || deltaCol == 0) {
                return IsPathClear(fromRow, fromCol, toRow, toCol);
            }
            return false;

        case 6: // Queen
            if (deltaRow == 0 || deltaCol == 0 || abs(deltaRow) == abs(deltaCol)) {
                return IsPathClear(fromRow, fromCol, toRow, toCol);
            }
            return false;

        case 1: // King
            if (abs(deltaRow) <= 1 && abs(deltaCol) <= 1) return true; // Regular king move
            // Castling
            if (deltaRow == 0 && abs(deltaCol) == 2) {
                int rookFrom = (deltaCol > 0) ? from + 3 : from - 4;
                int rookTo = (deltaCol > 0) ? from + 1 : from - 1;
                if ((squares[rookFrom] & 7) == 5 && IsPathClear(fromRow, fromCol, toRow, rookTo)) {
                    // Ensure no squares the king passes through are under attack
                    for (int col = fromCol; col != toCol; col += (deltaCol > 0 ? 1 : -1)) {
                        if (IsKingInCheck(fromRow * BOARD_SIZE + col)) {
                            return false;
                        }
                    }
                    return true;
                }
            }
            return false;

        default:
            return false;
    }
}

bool IsPathClear(int fromRow, int fromCol, int toRow, int toCol) {
    int deltaRow = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int deltaCol = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;

    int currentRow = fromRow + deltaRow;
    int currentCol = fromCol + deltaCol;

    while (currentRow != toRow || currentCol != toCol) {
        if (squares[currentRow * BOARD_SIZE + currentCol] != p.none) {
            return false; // Path obstructed
        }
        currentRow += deltaRow;
        currentCol += deltaCol;
    }
    return true;
}

bool IsKingInCheck(int kingPosition) {
    int opponentColor = (currentTurn == p.white) ? p.black : p.white;

    for (int i = 0; i < 64; ++i) {
        int piece = squares[i];
        if ((piece & (p.white | p.black)) == opponentColor) {
            if (IsValidMove(piece, i, kingPosition)) {
                return true; // The king is under attack
            }
        }
    }
    return false;
}

bool NeedsPromotion(int piece, int squareIndex, int boardSize, int currentTurn) {
    // Check if the piece is a pawn
    const int pawnType = 2; // Corresponding to `p.pawn`
    Piece p;
    if ((piece & 7) != pawnType) {
        return false; // Not a pawn
    }

    // Check if the pawn is on the promotion rank
    int row = squareIndex / boardSize;
    if ((currentTurn == p.white && row == 0) || (currentTurn == p.black && row == boardSize - 1)) {
        return true;
    }

    return false;
}

int FindKingPosition() {
    int kingType = p.king | currentTurn;
    for (int i = 0; i < 64; ++i) {
        if (squares[i] == kingType) {
            return i;
        }
    }
    return -1; // King not found (error case)
}

bool IsCheckmate() {
    int kingPosition = FindKingPosition();
    if (kingPosition == -1) {
        std::cerr << "Error: King not found on the board." << std::endl;
        return false;
    }

    if (!IsKingInCheck(kingPosition)) {
        // std::cout << "King is not in check. Not checkmate." << std::endl;
        return false;
    }

    for (int from = 0; from < 64; ++from) {
        int piece = squares[from];
        if ((piece & (p.white | p.black)) == currentTurn) {
            for (int to = 0; to < 64; ++to) {
                if (IsValidMove(piece, from, to)) {
                    // Simulate move
                    int savedPiece = squares[to];
                    squares[to] = piece;
                    squares[from] = p.none;

                    // Check if the king is still in check
                    int newKingPosition = (piece & 7) == p.king ? to : kingPosition;
                    bool stillInCheck = IsKingInCheck(newKingPosition);

                    // Undo the move
                    squares[from] = piece;
                    squares[to] = savedPiece;

                    if (!stillInCheck) {
                        std::cout << "Found a valid move from " << from << " to " << to << ". Not checkmate." << std::endl;
                        return false;
                    }
                }
            }
        }
    }

    std::cout << "No valid moves found. Checkmate!" << std::endl;
    return true;
}

bool IsStalemate() {
    // Find the current player's king position
    int kingPosition = FindKingPosition();
    if (kingPosition == -1) {
        std::cerr << "Error: King not found on the board." << std::endl;
        return false;
    }

    // If the king is in check, it's not stalemate
    if (IsKingInCheck(kingPosition)) {
        return false;
    }

    // Iterate over all pieces belonging to the current player
    for (int from = 0; from < 64; ++from) {
        int piece = squares[from];
        if ((piece & (p.white | p.black)) == currentTurn) {

            // Check all possible moves for this piece
            for (int to = 0; to < 64; ++to) {
                if (IsValidMove(piece, from, to)) {
                    return false; // Found a valid move
                }
            }
        }
    }

    // No valid moves and the king is not in check
    return true;
}

int getSquareIndex(int x, int y, int squareSize) {
    x -= boardX;
    y -= boardY;

    if (x < 0 || y < 0 || x >= squareSize * BOARD_SIZE || y >= squareSize * BOARD_SIZE) {
        return -1; // Invalid click (outside the board area)
    }

    int col = x / squareSize;
    int row = y / squareSize;
    return row * BOARD_SIZE + col;
}

};


void RunCheckmateTests(Board& board) {
    struct TestCase {
        std::string description;
        std::string fen;
        bool expectedCheckmate;
        bool expectedStalemate;
    };

    std::vector<TestCase> testCases = {
        {"Simple Checkmate: Black King Cornered", 
         "6k1/5ppp/8/8/8/8/7Q/7K w - - 0 1", true, false},
        {"Back-Rank Checkmate: Rook Mate", 
         "7k/5ppp/8/8/8/8/6RR/7K w - - 0 1", true, false},
        {"Smothered Mate: Knight", 
         "6k1/8/8/8/8/8/5N1P/6K1 w - - 0 1", true, false},
        {"Stalemate: King Has No Moves", 
         "7k/5ppp/8/8/8/8/7Q/7K w - - 0 1", false, true},
        {"King Has Escape Square", 
         "8/8/8/8/8/5k2/4R3/7K w - - 0 1", false, false}
    };

    for (const auto& test : testCases) {
        board.LoadPositionFromFen(test.fen);
        bool isCheckmate = board.IsCheckmate();
        bool isStalemate = board.IsStalemate();

        std::cout << "Test: " << test.description << "\n";
        std::cout << "FEN: " << test.fen << "\n";
        std::cout << "Expected Checkmate: " << test.expectedCheckmate << ", Actual: " << isCheckmate << "\n";
        std::cout << "Expected Stalemate: " << test.expectedStalemate << ", Actual: " << isStalemate << "\n";
        std::cout << (isCheckmate == test.expectedCheckmate && isStalemate == test.expectedStalemate ? "PASS" : "FAIL") << "\n\n";
    }
}

void drawChessboard(SDL_Renderer* renderer, const std::vector<int>& validMoves, int pickedSquare = -1) {
    // Colors
    SDL_Color lightSquareColor = {240, 217, 181, 255}; // Original light square
    SDL_Color darkSquareColor = {181, 136, 99, 255};  // Original dark square
    SDL_Color highlightLight = {255, 100, 100, 100};  // Semi-transparent red tint for light squares
    SDL_Color highlightDark = {200, 50, 50, 150};     // Semi-transparent darker red tint for dark squares
    SDL_Color pickedSquareColor = {219, 157, 70, 255}; // Golden color for picked square (semi-transparent)

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            int squareIndex = row * BOARD_SIZE + col;

            // Determine the base color of the square
            SDL_Color baseColor = (row + col) % 2 == 0 ? lightSquareColor : darkSquareColor;

            // Highlight picked square
            if (squareIndex == pickedSquare) {
                SDL_SetRenderDrawColor(renderer, pickedSquareColor.r, pickedSquareColor.g, pickedSquareColor.b, pickedSquareColor.a);
            }
            // Highlight valid moves with alternating tints
            else if (std::find(validMoves.begin(), validMoves.end(), squareIndex) != validMoves.end()) {
                SDL_Color highlightColor = (row + col) % 2 == 0 ? highlightLight : highlightDark;
                SDL_SetRenderDrawColor(renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
            }
            // Default square color
            else {
                SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            }

            // Calculate square position
            int x = boardX + col * squareSize;  // Center horizontally
            int y = boardY + row * squareSize;  // Keep vertically aligned

            // Render the square
            SDL_Rect square = {
                x,
                y,
                squareSize,
                squareSize
            };

            SDL_RenderFillRect(renderer, &square);
        }
    }
}


void drawPieces(SDL_Renderer* renderer, Board& board, std::unordered_map<int, SDL_Texture*>& textures) {
    for (int i = 0; i < 64; ++i) {
        int piece = board.squares[i];
        if (piece == 0) continue; 

        int row = i / BOARD_SIZE;
        int col = i % BOARD_SIZE;

        // Calculate square position
        int x = boardX + col * squareSize;  // Center horizontally
        int y = boardY + row * squareSize;  // Keep vertically aligned

        SDL_Rect dstRect = {x, y, squareSize, squareSize};

        if (textures.count(piece)) {
            SDL_RenderCopy(renderer, textures[piece], nullptr, &dstRect);
        }
    }
}

int showPromotionDialog(SDL_Renderer* renderer, std::unordered_map<int, SDL_Texture*>& textures, int currentTurn) {
    const int dialogWidth = 200;
    const int dialogHeight = 100;
    const int dialogX = (WINDOW_WIDTH - dialogWidth) / 2;
    const int dialogY = (WINDOW_HEIGHT - dialogHeight) / 2;
    Piece p;

    // Draw the dialog box
    SDL_Rect dialogRect = {dialogX, dialogY, dialogWidth, dialogHeight};
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &dialogRect);

    // Draw promotion options
    int pieceTypes[] = {p.queen, p.rook, p.bishop, p.knight};
    SDL_Rect pieceRects[4];
    for (int i = 0; i < 4; ++i) {
        pieceRects[i] = {dialogX + i * 50, dialogY + 25, 50, 50}; // Position the pieces in the dialog
        int piece = pieceTypes[i] | currentTurn; // Add color to the piece
        SDL_RenderCopy(renderer, textures[piece], nullptr, &pieceRects[i]);
    }
    SDL_RenderPresent(renderer);

    // Wait for user selection
    SDL_Event event;
    while (true) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return -1; // Exit if the user quits
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;

                // Create a valid SDL_Point
                SDL_Point mousePoint = {mouseX, mouseY};

                // Check if the mouse click is inside any of the piece rectangles
                for (int i = 0; i < 4; ++i) {
                    if (SDL_PointInRect(&mousePoint, &pieceRects[i])) {
                        return pieceTypes[i]; // Return the selected piece type
                    }
                }
            }
        }
    }
}

void drawRoundedSquare(SDL_Renderer* renderer) {
    // Set color for the rounded square
    SDL_Color squareColor = {100, 100, 100, 255}; // Example color (gray)

    // Position and size of the rounded square
    int roundedSquareX = boardX + BOARD_WIDTH + 50; // To the right of the chessboard
    int roundedSquareY = boardY + 100;            // Some distance below the top of the window
    int roundedSquareWidth = 200;                 // Width of the square
    int roundedSquareHeight = 200;                // Height of the square
    int cornerRadius = 20;                        // Radius of the rounded corners

    // Set the draw color
    SDL_SetRenderDrawColor(renderer, squareColor.r, squareColor.g, squareColor.b, squareColor.a);

    // Draw the center rectangle
    SDL_Rect centerRect = {roundedSquareX + cornerRadius, roundedSquareY + cornerRadius, roundedSquareWidth - 2 * cornerRadius, roundedSquareHeight - 2 * cornerRadius};
    SDL_RenderFillRect(renderer, &centerRect);

    // Draw top and bottom rectangles
    SDL_Rect topRect = {roundedSquareX + cornerRadius, roundedSquareY, roundedSquareWidth - 2 * cornerRadius, cornerRadius};
    SDL_Rect bottomRect = {roundedSquareX + cornerRadius, roundedSquareY + roundedSquareHeight - cornerRadius, roundedSquareWidth - 2 * cornerRadius, cornerRadius};
    SDL_RenderFillRect(renderer, &topRect);
    SDL_RenderFillRect(renderer, &bottomRect);

    // Draw left and right rectangles
    SDL_Rect leftRect = {roundedSquareX, roundedSquareY + cornerRadius, cornerRadius, roundedSquareHeight - 2 * cornerRadius};
    SDL_Rect rightRect = {roundedSquareX + roundedSquareWidth - cornerRadius, roundedSquareY + cornerRadius, cornerRadius, roundedSquareHeight - 2 * cornerRadius};
    SDL_RenderFillRect(renderer, &leftRect);
    SDL_RenderFillRect(renderer, &rightRect);

    // Optionally, draw the corners using circles (if needed)
}

void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        std::cerr << "Failed to create text surface: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create text texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect textRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


int main(int argc, char* argv[]) {
    bool isDragging = false;
    int draggedPiece = 0;
    int draggedFromSquare = -1;
    int mouseX = 0, mouseY = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chessboard",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Failed to initialize SDL2_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    if (TTF_Init() == -1) {
    std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
    return -1;
    }


    TTF_Font* font = TTF_OpenFont("fonts/arial.ttf", 24); // Font size 24
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return -1;
    }


    Piece p;
    BoardStateList state;
    std::unordered_map<int, SDL_Texture*> textures;
    
    textures[p.white | p.king] = IMG_LoadTexture(renderer, "res/Pieces/w_king_2x.png");
    textures[p.white | p.queen] = IMG_LoadTexture(renderer, "res/Pieces/w_queen_2x.png");
    textures[p.white | p.rook] = IMG_LoadTexture(renderer, "res/Pieces/w_rook_2x.png");
    textures[p.white | p.bishop] = IMG_LoadTexture(renderer, "res/Pieces/w_bishop_2x.png");
    textures[p.white | p.knight] = IMG_LoadTexture(renderer, "res/Pieces/w_knight_2x.png");
    textures[p.white | p.pawn] = IMG_LoadTexture(renderer, "res/Pieces/w_pawn_2x.png");

    textures[p.black | p.king] = IMG_LoadTexture(renderer, "res/Pieces/b_king_2x.png");
    textures[p.black | p.queen] = IMG_LoadTexture(renderer, "res/Pieces/b_queen_2x.png");
    textures[p.black | p.rook] = IMG_LoadTexture(renderer, "res/Pieces/b_rook_2x.png");
    textures[p.black | p.bishop] = IMG_LoadTexture(renderer, "res/Pieces/b_bishop_2x.png");
    textures[p.black | p.knight] = IMG_LoadTexture(renderer, "res/Pieces/b_knight_2x.png");
    textures[p.black | p.pawn] = IMG_LoadTexture(renderer, "res/Pieces/b_pawn_2x.png");

    for (auto& [key, texture] : textures) {
        if (!texture) {
            std::cerr << "Failed to load texture for piece: " << key << " - " << IMG_GetError() << std::endl;
        }
    }

    Board board;
    BoardStateList boardStateList;
    boardStateList.AddState(board.GetFenFromPosition());

    bool running = true;
    SDL_Event event;
    int squareSize = BOARD_WIDTH / BOARD_SIZE;
    int startSquare = -1; // Stores the index of the starting square
    int endSquare = -1;   // Stores the index of the ending square

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int squareIndex = startSquare = board.getSquareIndex(event.button.x, event.button.y, squareSize);
                        int piece = board.squares[squareIndex];
                        std::cout<<"is latest state A: "<<isAtLatestState<<std::endl;
                            if (piece != board.p.none && (piece & (board.p.white | board.p.black)) == board.currentTurn && isAtLatestState) {
                            isDragging = true;
                            draggedFromSquare = squareIndex;
                            draggedPiece = board.squares[squareIndex];
                            board.squares[squareIndex] = board.p.none;
                            mouseX = event.button.x;
                            mouseY = event.button.y;

                            // Calculate valid moves for the selected piece
                            p.validMoves.clear();
                            for (int target = 0; target < 64; ++target) {
                                if (board.IsValidMove(draggedPiece, squareIndex, target)) {
                                    p.validMoves.push_back(target);
                                }
                            }
                        }
                        std::cout<<"is latest state B: "<<isAtLatestState<<std::endl;
                        
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT && isDragging) {
                        int squareIndex = endSquare = board.getSquareIndex(event.button.x, event.button.y, squareSize);
                        if (board.IsValidMove(draggedPiece, draggedFromSquare, squareIndex)) {
                            board.squares[squareIndex] = draggedPiece;

                            int movedPiece = board.squares[squareIndex]; // Piece after move
                            if (board.IsCheckmate()) {
                                std::string winner = (board.currentTurn == board.p.white) ? "Black" : "White";
                                std::cout << "Checkmate! " << winner << " wins!" << std::endl;
                                running = false; // End the game
                            }

                            if (board.NeedsPromotion(movedPiece, squareIndex, BOARD_SIZE, board.currentTurn)) {
                                std::cout << "Pawn needs promotion!" << std::endl;
                                int promotedPiece = showPromotionDialog(renderer, textures, board.currentTurn);
                                if (promotedPiece > 0) {
                                    board.squares[squareIndex] = promotedPiece | board.currentTurn; // Replace with promoted piece
                                }
                            }
                            boardStateList.AddState(board.GetFenFromPosition());  // Add the current state before the move                        
                            board.SwitchTurn();
                        } else {
                            board.squares[draggedFromSquare] = draggedPiece;
                        }
                        isDragging = false;
                        draggedFromSquare = -1;
                        // if(startSquare != endSquare){
                        //     std::string updatedFen = board.GetFenFromPosition();
                        //     std::cout << "Updated FEN: " << updatedFen << std::endl;
                        // }
                    }
                    p.validMoves.clear();
                    break;
                case SDL_MOUSEMOTION:
                    if (isDragging) {
                        mouseX = event.motion.x;
                        mouseY = event.motion.y;
                    }
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_l) { // Press 'L' to load a custom FEN
                        std::string customFen = "6k1/5ppp/8/8/8/5Q2/6PP/6K1 w - - 0 1";
                        board.LoadPositionFromFen(customFen);
                        std::cout << "Loaded FEN: " << customFen << std::endl;
                    }
                    std::string previousState;
                    if (event.key.keysym.sym == SDLK_u) {  // Press 'U' to undo
                        std::string previousState;
                        if (boardStateList.Undo(previousState)) {
                            board.LoadPositionFromFen(previousState);
                        } else {
                            std::cout << "Nothing to undo!" << std::endl;
                        }
                    }
                    if (event.key.keysym.sym == SDLK_r) {  // Press 'R' to redo
                        std::string nextState;
                        if (boardStateList.Redo(nextState)) {
                            board.LoadPositionFromFen(nextState);
                        } else {
                            std::cout << "Nothing to redo!" << std::endl;
                        }
                    }
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 48, 46, 43, 255);
        SDL_RenderClear(renderer);

        drawChessboard(renderer, p.validMoves, draggedFromSquare);
        drawPieces(renderer, board, textures);

        bool isWhiteTurn = true; // Track whose turn it is

        // Inside the render loop
        SDL_Color textColor = {255, 255, 255, 255}; // White color for text
        const char* turnText = (board.currentTurn == p.white) ? "White's Turn" : "Black's Turn";
        renderText(renderer, font, turnText, 50, 100, textColor); // Display text at (50, 100)


        if (isDragging) {
            SDL_Rect dstRect = {mouseX - squareSize / 2, mouseY - squareSize / 2, squareSize, squareSize};
            SDL_RenderCopy(renderer, textures[draggedPiece], nullptr, &dstRect);
        }

        SDL_RenderPresent(renderer);
    }

    for (auto& [key, texture] : textures) {
        SDL_DestroyTexture(texture);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}