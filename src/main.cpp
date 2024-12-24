#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <vector>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;
const int BOARD_SIZE = 8;
const int TOTAL_SQUARES = BOARD_SIZE * BOARD_SIZE;

class Piece {
public:
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
        std::string playerTurn = (currentTurn == p.white) ? "White's Turn" : "Black's Turn";
        std::cout<<playerTurn<<std::endl;
    }

};

std::vector<int> validMoves;

bool IsPathClear(int fromRow, int fromCol, int toRow, int toCol, Board& board) {
    int deltaRow = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    int deltaCol = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;

    int currentRow = fromRow + deltaRow;
    int currentCol = fromCol + deltaCol;

    while (currentRow != toRow || currentCol != toCol) {
        if (board.squares[currentRow * BOARD_SIZE + currentCol] != board.p.none) {
            return false; // Path obstructed
        }
        currentRow += deltaRow;
        currentCol += deltaCol;
    }
    return true;
}

bool IsValidMove(int piece, int from, int to, Board& board) {
    if (from == to) return false; // Can't move to the same square

    int fromRow = from / BOARD_SIZE;
    int fromCol = from % BOARD_SIZE;
    int toRow = to / BOARD_SIZE;
    int toCol = to % BOARD_SIZE;

    int deltaRow = toRow - fromRow;
    int deltaCol = toCol - fromCol;

    int pieceType = piece & 7; // Mask out color bits
    int pieceColor = piece & (board.p.white | board.p.black);

    // Prevent capturing your own piece
    if (board.squares[to] != board.p.none && (board.squares[to] & (board.p.white | board.p.black)) == pieceColor) {
        return false;
    }

    switch (pieceType) {
        case 2: // Pawn
            if (pieceColor == board.p.white) {
                if (deltaRow == -1 && deltaCol == 0 && board.squares[to] == board.p.none) return true; // Move forward
                if (fromRow == 6 && deltaRow == -2 && deltaCol == 0 && board.squares[to] == board.p.none) return true; // Double move
                if (deltaRow == -1 && abs(deltaCol) == 1 && board.squares[to] & board.p.black) return true; // Capture
            } else { // Black pawn
                if (deltaRow == 1 && deltaCol == 0 && board.squares[to] == board.p.none) return true; // Move forward
                if (fromRow == 1 && deltaRow == 2 && deltaCol == 0 && board.squares[to] == board.p.none) return true; // Double move
                if (deltaRow == 1 && abs(deltaCol) == 1 && board.squares[to] & board.p.white) return true; // Capture
            }
            return false;

        case 3: // Knight
            return (abs(deltaRow) == 2 && abs(deltaCol) == 1) || (abs(deltaRow) == 1 && abs(deltaCol) == 2);

        case 4: // Bishop
            if (abs(deltaRow) == abs(deltaCol)) {
                return IsPathClear(fromRow, fromCol, toRow, toCol, board);
            }
            return false;

        case 5: // Rook
            if (deltaRow == 0 || deltaCol == 0) {
                return IsPathClear(fromRow, fromCol, toRow, toCol, board);
            }
            return false;

        case 6: // Queen
            if (deltaRow == 0 || deltaCol == 0 || abs(deltaRow) == abs(deltaCol)) {
                return IsPathClear(fromRow, fromCol, toRow, toCol, board);
            }
            return false;

        case 1: // King
            return abs(deltaRow) <= 1 && abs(deltaCol) <= 1;

        default:
            return false;
    }
}

void drawChessboard(SDL_Renderer* renderer, const std::vector<int>& validMoves, int pickedSquare = -1) {
    int squareSize = WINDOW_WIDTH / BOARD_SIZE;

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

            // Render the square
            SDL_Rect square = {
                col * squareSize,
                row * squareSize,
                squareSize,
                squareSize
            };

            SDL_RenderFillRect(renderer, &square);
        }
    }
}


void drawPieces(SDL_Renderer* renderer, Board& board, std::unordered_map<int, SDL_Texture*>& textures) {
    int squareSize = WINDOW_WIDTH / BOARD_SIZE;

    for (int i = 0; i < 64; ++i) {
        int piece = board.squares[i];
        if (piece == 0) continue; 

        int row = i / BOARD_SIZE;
        int col = i % BOARD_SIZE;

        SDL_Rect dstRect = {col * squareSize, row * squareSize, squareSize, squareSize};

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


bool NeedsPromotion(int piece, int squareIndex, int boardSize, int currentTurn) {
    // Check if the piece is a pawn
    const int pawnType = 2; // Corresponding to `Piece::pawn`
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

bool IsKingInCheck(int kingPosition, Board& board) {
    int opponentColor = (board.currentTurn == board.p.white) ? board.p.black : board.p.white;

    for (int i = 0; i < 64; ++i) {
        int piece = board.squares[i];
        if ((piece & (board.p.white | board.p.black)) == opponentColor) {
            if (IsValidMove(piece, i, kingPosition, board)) {
                return true; // The king is under attack
            }
        }
    }
    return false;
}

int FindKingPosition(Board& board) {
    int kingType = board.p.king | board.currentTurn;
    for (int i = 0; i < 64; ++i) {
        if (board.squares[i] == kingType) {
            return i;
        }
    }
    return -1; // King not found (error case)
}

bool IsCheckmate(Board& board) {
    int kingPosition = FindKingPosition(board);
    if (kingPosition == -1) return false; // Invalid board state

    if (!IsKingInCheck(kingPosition, board)) {
        return false; // King is not in check
    }

    // Check if any valid move can get the king out of check
    for (int from = 0; from < 64; ++from) {
        if ((board.squares[from] & (board.p.white | board.p.black)) == board.currentTurn) {
            for (int to = 0; to < 64; ++to) {
                int piece = board.squares[from];

                if (IsValidMove(piece, from, to, board)) {
                    // Temporarily make the move
                    int savedPiece = board.squares[to];
                    board.squares[to] = piece;
                    board.squares[from] = board.p.none;

                    bool stillInCheck = IsKingInCheck(FindKingPosition(board), board);

                    // Undo the move
                    board.squares[from] = piece;
                    board.squares[to] = savedPiece;

                    if (!stillInCheck) {
                        return false; // Found a valid move that gets out of check
                    }
                }
            }
        }
    }

    return true; // No valid moves; the king is checkmated
}


int getSquareIndex(int x, int y, int squareSize) {
    int col = x / squareSize;
    int row = y / squareSize;
    return row * BOARD_SIZE + col;
}

void gitVersionControlTester(){
    std::cout<<"This is bullshit"<<std::endl;
    std::cout<<"This is another load of bullshit"<<std::endl;
}

int main(int argc, char* argv[]) {
    std::cout<<"This is a change to test git version control"<<std::endl;
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

    Piece p;
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

    bool running = true;
    SDL_Event event;
    int squareSize = WINDOW_WIDTH / BOARD_SIZE;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int squareIndex = getSquareIndex(event.button.x, event.button.y, squareSize);
                        int piece = board.squares[squareIndex];
                        if (piece != board.p.none && (piece & (board.p.white | board.p.black)) == board.currentTurn) {
                            isDragging = true;
                            draggedFromSquare = squareIndex;
                            draggedPiece = board.squares[squareIndex];
                            board.squares[squareIndex] = board.p.none;
                            mouseX = event.button.x;
                            mouseY = event.button.y;

                            // Calculate valid moves for the selected piece
                            validMoves.clear();
                            for (int target = 0; target < 64; ++target) {
                                if (IsValidMove(draggedPiece, squareIndex, target, board)) {
                                    validMoves.push_back(target);
                                }
                            }
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT && isDragging) {
                        int squareIndex = getSquareIndex(event.button.x, event.button.y, squareSize);
                        if (IsValidMove(draggedPiece, draggedFromSquare, squareIndex, board)) {
                            board.squares[squareIndex] = draggedPiece;

                            int movedPiece = board.squares[squareIndex]; // Piece after move

                            if (IsCheckmate(board)) {
                                std::string winner = (board.currentTurn == board.p.white) ? "Black" : "White";
                                std::cout << "Checkmate! " << winner << " wins!" << std::endl;
                                running = false; // End the game
                                break;
                            }

                            if (NeedsPromotion(movedPiece, squareIndex, BOARD_SIZE, board.currentTurn)) {
                                std::cout << "Pawn needs promotion!" << std::endl;
                                int promotedPiece = showPromotionDialog(renderer, textures, board.currentTurn);
                                if (promotedPiece > 0) {
                                    board.squares[squareIndex] = promotedPiece | board.currentTurn; // Replace with promoted piece
                                }
                            }

                            board.SwitchTurn();
                        } else {
                            board.squares[draggedFromSquare] = draggedPiece;
                        }
                        isDragging = false;
                        draggedFromSquare = -1;
                        std::string updatedFen = board.GetFenFromPosition();
                        std::cout << "Updated FEN: " << updatedFen << std::endl;
                    }
                    validMoves.clear();
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
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        drawChessboard(renderer, validMoves, draggedFromSquare);
        drawPieces(renderer, board, textures);

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
    IMG_Quit();
    SDL_Quit();

    return 0;
}