#ifndef GAMELOGIC_H_INCLUDED
#define GAMELOGIC_H_INCLUDED
#include <types.h>
#include <data.h>

#define TILEMASK 0xF //Get tile type from tile data by doing data value & TILEMASK
#define TILEFLAGMASK 0xF0 //Get tile flags from tile data by doing data value & TILEMASK

#define TILE_UNCOVER_STACKSIZE 1024 //Max stack size for uncovered tiles (actual size is double that as the stack is u16)

extern struct GameData mineGameData;

#define GXYINDEX(x,y) ((x)+(y)*mineGameData.grid.width)

enum PlayerStatus
{
    PST_NOTSTARTED, //Game not yet started, before the first press
    PST_PLAYING,    //After the first press
    PST_DEAD,       //Game over
    PST_VICTORY     //Player won the game
};

//Values and bitwise flags for grid data
enum TileType
{
    TT_NONE = 0,    //Tile without a number
    TT_N1 = 1,      //Tile with number 1
    TT_N2 = 2,      //Tile with number 2
    TT_N3 = 3,      //Tile with number 3
    TT_N4 = 4,      //Tile with number 4
    TT_N5 = 5,      //Tile with number 5
    TT_N6 = 6,      //Tile with number 6
    TT_N7 = 7,      //Tile with number 7
    TT_N8 = 8,      //Tile with number 8
    TT_MINE = 9,    //Tile with a mine

    TT_COVERED = 16, //Tile is covered (bitwise flag)
    TT_FLAGGED = 32, //Tile is flagged (bitwise flag)

    TT_INVALID = 128 //Tile is invalid
};

struct MineGrid
{
    u16 width;
    u16 height;
    u16 totalSize;
    s16 mineCount;
    u8* data; //Dynamically allocated u8 array of size width*height
};

struct GameData
{
    struct MineGrid grid;
    s16 remainingMineCount; //On-screen mine count, flagging mines decreases it by 1
    enum PlayerStatus status;
    fix32 gameTime;
    u16* uncoverStack; //Dynamically allocated u16 array of size width*height
    u16 uncoverStackPos; //Current position in the uncoverStack
};

/**
 * \brief
 *      Try uncovering or flagging selected tile
 * 
 * \param rightclick
 *      if TRUE, flag the selected tile, otherwise uncover it 
 * \param isChord
 *      if TRUE, the click is a chord (reveals all the tiles if the correct flag amount is present)
*/
void logic_clickTile(bool rightclick, bool isChord);

/**
 * \brief
 *      Draw text on the middle HUD display
 * 
 * \param text
 *      Text to display on the HUD limited to 8 characters.
 *      If NULL, the text is picked based on current player status
*/
void logic_drawText(const char* text);

/**
 * \brief
 *      Draw borders around the minefield, must be called after logic_startGame()
*/
void logic_drawBorders(void);

/**
 * \brief
 *      Draw a tile on the minefield with relative coordinates
 * 
 * \param tile
 *      Grid tile from MineGrid data array
 * \param x
 *      X coordinate of a tile on the minefield
 * \param y
 *      Y coordinate of a tile on the minefield
*/
void logic_drawTile(u8 tile, u16 x, u16 y);

/**
 * \brief
 *      Initialize variables, generate minefield, configure palettes
*/
void logic_startGame(void);

/**
 * \brief
 *      Move in-game cursor
 * 
 * \param button
 *      SGDK button constant corresponding to a pressed dpad key
*/
void logic_moveCursor(u16 button);

//Returns place of current lowest time (0 - none, 1-3 - top 3) and puts time in leaderboard format into the given pointer
u16 logic_calculateScorePlace(u16* timePtr);

void logic_stop();

#endif