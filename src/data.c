#include <data.h>
#include <gamestate.h>
#include <menustate.h>
#include <scorestate.h>
#include <mouse.h>
#include <resources.h>

struct GameState states[STATE_COUNT];
u8 currentState = ST_GAMESTATE;

bool randomNoPattern = TRUE;

const char* versionStr = "RC 2";

u16 newPalette[64] = {0};

struct VidReservedImage vimages[VIMAGE_MAXCOUNT]; //Array of image data with reserved VRAM tile positions

int vImageCount = 0;

struct GameSettings settings;

bool isChangingState = FALSE;

struct HighScores lowestTimes;

//Initialize settings to default values
void initSettings(void)
{
    settings.difficulty = DL_Intermediate;
    settings.width = 16;
    settings.height = 16;
    settings.mineCount = 40;
}

//Initialize all sounds
void data_initsfx(void)
{
    XGM_setPCM(SFX_CLICK,sfx_click,sizeof(sfx_click));
    XGM_setPCM(SFX_UNCOVER,sfx_uncover,sizeof(sfx_uncover));
}

//Initialize settings and lowest times to default values
void initData(void)
{
    initSettings();
    for(u16 i=0; i<3; i++)
    {
        lowestTimes.topBeginner[i].score = MAX_U16;
        memset(lowestTimes.topBeginner[i].name,'\0',sizeof(lowestTimes.topBeginner[i].name));
        lowestTimes.topIntermediate[i].score = MAX_U16;
        memset(lowestTimes.topIntermediate[i].name,'\0',sizeof(lowestTimes.topIntermediate[i].name));
        lowestTimes.topExpert[i].score = MAX_U16;
        memset(lowestTimes.topExpert[i].name,'\0',sizeof(lowestTimes.topExpert[i].name));
    }
    lowestTimes.lastScore = MAX_U32;
}

//Assign state callbacks to game states
void setupStates(void)
{
    states[ST_GAMESTATE].init = &gamestate_init;
    states[ST_GAMESTATE].update = &gamestate_update;
    states[ST_GAMESTATE].joyevent = &gamestate_joyevent;
    states[ST_GAMESTATE].stop = &gamestate_stop;

    states[ST_MENUSTATE].init = &menustate_init;
    states[ST_MENUSTATE].update = &menustate_update;
    states[ST_MENUSTATE].joyevent = &menustate_joyevent;
    states[ST_MENUSTATE].stop = &menustate_stop;

    states[ST_SCORESTATE].init = &scorestate_init;
    states[ST_SCORESTATE].update = &scorestate_update;
    states[ST_SCORESTATE].joyevent = &scorestate_joyevent;
    states[ST_SCORESTATE].stop = &scorestate_stop;
}

//Allocate space in VRAM for an image and return a pointer to a VidReservedImage struct
VidImagePtr reserveVImage(const Image* img, bool preload)
{
    if(vImageCount>=VIMAGE_MAXCOUNT)
    {
        SYS_die("Too many images reserved");
    }
    VidImagePtr vidimg = &vimages[vImageCount];
    if(preload)
        VDP_loadTileSet(img->tileset,curTileInd,DMA_QUEUE);
    vidimg->img = img;
    vidimg->vPos = curTileInd;
    curTileInd += img->tileset->numTile;
    vImageCount++;
    return vidimg;
}

//Initialize the game with a given state (should be called only once)
void initState(enum States newState)
{
    isChangingState = TRUE;
    if(newState >= STATE_COUNT)
    {
        SYS_die("Invalid game state!");
    }
    PAL_setColors(0,palette_black,64,CPU);
    vImageCount = 0;
    curTileInd = TILE_USER_INDEX;
    currentState = newState;
    memset(newPalette,0,sizeof(newPalette));
    newPalette[15] = RGB24_TO_VDPCOLOR(0xEEEEEE);
    XGM_stopPlay();
    if(states[currentState].init)
        states[currentState].init();
    if(mouse_isEnabled())
        mouse_changeState(newState);
    PAL_fadeIn(0,63,newPalette,15,FALSE);
    isChangingState = FALSE;
}

void changeState(enum States newState)
{
    isChangingState = TRUE;
    if(newState >= STATE_COUNT)
    {
        SYS_die("Invalid game state!");
    }
    PAL_fadeOut(0,63,10,FALSE);
    if(states[currentState].stop)
        states[currentState].stop();
    VDP_clearPlane(BG_A,TRUE);
    VDP_clearPlane(BG_B,TRUE);
    VDP_clearSprites();
    SPR_defragVRAM();
    SPR_update();
    vImageCount = 0;
    curTileInd = TILE_USER_INDEX;
    currentState = newState;
    memset(newPalette,0,sizeof(newPalette));
    newPalette[15] = RGB24_TO_VDPCOLOR(0xEEEEEE);
    XGM_stopPlay();
    if(states[currentState].init)
        states[currentState].init();
    if(mouse_isEnabled())
        mouse_changeState(newState);
    PAL_fadeIn(0,63,newPalette,15,FALSE);
    isChangingState = FALSE;
}