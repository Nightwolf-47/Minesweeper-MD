#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED
#include <genesis.h>

#define MIN_WIDTH_HEIGHT 5 //Minimal width and height

#define STATE_COUNT 3 //Amount of game states

#define VIMAGE_MAXCOUNT 20 //Max amount of VidImages

#define MAXGRIDSIZE 836 //Max size of minesweeper grid (38*22)

#define GETCENTERX(str) (20-(strlen(str)>>1)) //Get the X position to make string centered

#define GETRIGHTX(str,offs) (40-strlen(str)-(offs)) //Get the X position to make string right-aligned with offset

#define SCORE_NAME_LEN 7


#define SFX_CLICK 64

#define SFX_UNCOVER 65

enum States { //List of game states
    ST_GAMESTATE,
    ST_MENUSTATE,
    ST_SCORESTATE
};

struct VidReservedImage //Structure containing a pointer to an image and VRAM position it should be drawn at (with VDP_drawImageEx TILE_ATTR_FULL)
{
    u16 vPos;
    const Image* img;
};

typedef struct VidReservedImage* VidImagePtr; //Shorter type for pointer to VidReservedImage

struct GameState { //Game state structure (not to be confused with ST_GAMESTATE)
    void (*init)(void);
    void (*stop)(void);
    void (*update)(fix32); //argument - fix32 dt
    void (*joyevent)(u16,u16,u16); //arguments - u16 joy, u16 changed, u16 state
};

//Minesweeper difficulty levels
enum DifficultyLevel {
    DL_Beginner = 0,
    DL_Intermediate,
    DL_Expert,
    DL_Custom
};

struct GameSettings {
    enum DifficultyLevel difficulty;
    u16 width;
    u16 height;
    u16 mineCount;
};

struct TopScore {
    u16 score;
    char name[SCORE_NAME_LEN];
};

struct HighScores {
    struct TopScore topBeginner[3];
    struct TopScore topIntermediate[3];
    struct TopScore topExpert[3];
    u32 lastScore;
};

void initSettings(void);

void data_initsfx(void);

void initData(void);

void setupStates(void);

void initState(enum States newState);

void changeState(enum States newState);

VidImagePtr reserveVImage(const Image* img, bool preload);

extern struct GameState states[STATE_COUNT];
extern u8 currentState; //Current game state

extern bool randomNoPattern; //If TRUE, random() is called every frame to prevent the RNG from returning the same values every time

extern const char* versionStr; //Version string, shows up in About menu

extern u16 newPalette[64]; //New palette, has to be set in init() function of a given state

extern struct GameSettings settings; //In-game settings

extern bool isChangingState; //If true, changeState function is currently in progress

extern struct HighScores lowestTimes;

#endif