#include <gamelogic.h>
#include <data.h>
#include <genesis.h>
#include <resources.h>
#include <mouse.h>

#define PACKCoords(x,y) (((u16)((u8)(x))<<8) | (u8)(y))

struct GameData mineGameData;

s16 gridXOffset = 0;
s16 gridYOffset = 0;

VidImagePtr tileTextures[10]; //Stores tile textures (0-8 and 9 -> mine)
VidImagePtr coveredTileTex;
VidImagePtr flaggedTileTex;
VidImagePtr borderHTex;
VidImagePtr borderVTex;
VidImagePtr borderCTex;

s16 gameCursorX = 0;
s16 gameCursorY = 0;

u16 tileUncoveredCount = 0;

void touchedMine(void);

//Sets the width, height and mine count values depending on selected difficulty
void setupDifficulty(void)
{
    switch(settings.difficulty)
    {
        case DL_Beginner:
            mineGameData.grid.width = 9;
            mineGameData.grid.height = 9;
            mineGameData.grid.mineCount = mineGameData.remainingMineCount = 10;
            break;
        case DL_Intermediate:
            mineGameData.grid.width = 16;
            mineGameData.grid.height = 16;
            mineGameData.grid.mineCount = mineGameData.remainingMineCount = 40;
            break;
        case DL_Expert:
            mineGameData.grid.width = 30;
            mineGameData.grid.height = 16;
            mineGameData.grid.mineCount = mineGameData.remainingMineCount = 99;
            break;
        case DL_Custom:
            mineGameData.grid.width = settings.width;
            mineGameData.grid.height = settings.height;
            mineGameData.grid.mineCount = mineGameData.remainingMineCount = min(settings.mineCount, mineGameData.grid.width*mineGameData.grid.height-1);
            break;
        default:
            SYS_die("Invalid difficulty!");
            break;
    }
}

//Change selected tile color from green to red, used when the player lost the game
void setLosingPalette2(void)
{
    PAL_setColor(33,RGB24_TO_VDPCOLOR(0xAA2222));
    PAL_setColor(34,RGB24_TO_VDPCOLOR(0xEE2222));
    PAL_setColor(35,RGB24_TO_VDPCOLOR(0x662222));
    PAL_setColor(43,RGB24_TO_VDPCOLOR(0x882222));
}

//Returns TRUE if the tile is a mine, FALSE otherwise
bool isTileAMine(u16 x, u16 y)
{
    if(x >= mineGameData.grid.width || y >= mineGameData.grid.height)
        return FALSE;

    return (mineGameData.grid.data[GXYINDEX(x,y)] & TILEMASK) == TT_MINE;
}

//Returns TRUE if the tile is flagged, FALSE otherwise
bool isTileFlagged(u16 x, u16 y)
{
    if(x >= mineGameData.grid.width || y >= mineGameData.grid.height)
        return FALSE;

    return (mineGameData.grid.data[GXYINDEX(x,y)] & (TT_FLAGGED)) != 0;
}

/**
 * \brief
 *      Sets game cursor position, if position is invalid it restores the old one
 * 
 * \returns
 *      TRUE if position was changed, FALSE otherwise
*/
bool setGameCursor(u16 x, u16 y)
{
    if(x < mineGameData.grid.width && y < mineGameData.grid.height)
    {
        u16 oldx = gameCursorX;
        u16 oldy = gameCursorY;
        gameCursorX = x;
        gameCursorY = y;
        logic_drawTile(mineGameData.grid.data[GXYINDEX(oldx,oldy)],oldx,oldy);
        logic_drawTile(mineGameData.grid.data[GXYINDEX(gameCursorX,gameCursorY)],gameCursorX,gameCursorY);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * \brief
 *      Get the amount of neighboring tiles that are mines
 * 
 * \param x
 *      Base tile x coordinate
 * \param y
 *      Base tile y coordinate
 * \return
 *      Amount of mines nearby
*/
u16 getNeighboringMineCount(u16 x, u16 y)
{
    u16 count = 0;
    count += isTileAMine(x-1,y);
    count += isTileAMine(x-1,y-1);
    count += isTileAMine(x-1,y+1);
    count += isTileAMine(x,y-1);
    count += isTileAMine(x,y+1);
    count += isTileAMine(x+1,y);
    count += isTileAMine(x+1,y-1);
    count += isTileAMine(x+1,y+1);
    return count;
}

/**
 * \brief
 *      Modify tile number, only works on non-mine tiles and will fail if (tile_value + diff) > 8
 * 
 * \param x
 *      Tile x coordinate
 * \param y
 *      Tile y coordinate
 * \param diff
 *      Value to add to the tile (negative values subtract)
*/
void modifyTileVal(u16 x, u16 y, s16 diff)
{
    if(x >= mineGameData.grid.width || y >= mineGameData.grid.height)
        return;

    u16 index = GXYINDEX(x,y);
    u8 tileType = mineGameData.grid.data[index] & TILEMASK;
    if(tileType <= TT_N8 && (u16)(tileType+diff) <= TT_N8)
    {
        mineGameData.grid.data[index] += diff;
    }
}

/**
 * \brief
 *      Modify neighboring non-mine tile numbers
 * 
 * \param x
 *      Base tile x coordinate
 * \param y
 *      Base tile y coordinate
 * \param diff
 *      Value to add to every neighboring tile (negative values subtract)
*/
void modifyNeighborTileVals(u16 x, u16 y, s16 diff)
{
    modifyTileVal(x-1,y,diff);
    modifyTileVal(x-1,y-1,diff);
    modifyTileVal(x-1,y+1,diff);
    modifyTileVal(x,y-1,diff);
    modifyTileVal(x,y+1,diff);
    modifyTileVal(x+1,y,diff);
    modifyTileVal(x+1,y-1,diff);
    modifyTileVal(x+1,y+1,diff);
}

//Cover the tiles and fill the minefield with mines and numbers
void initializeGrid(void)
{
    u16 minesFilled = 0;
    //Fill the minefield with mines randomly, this may take a while
    while(minesFilled < mineGameData.grid.mineCount)
    {
        u16 x = random() % mineGameData.grid.width;
        u16 y = random() % mineGameData.grid.height;
        u16 index = GXYINDEX(x,y);
        if((mineGameData.grid.data[index] & TILEMASK) != TT_MINE)
        {
            mineGameData.grid.data[index] = TT_MINE;
            modifyNeighborTileVals(x,y,1);
            minesFilled++;
        }
    }
    
    for(u16 i=0; i<mineGameData.grid.totalSize; i++)
    {
        mineGameData.grid.data[i] |= TT_COVERED;
    }
}

/**
 * \brief
 *      Move the mine after first click so that player doesn't lose instantly. Should only be called before uncovering any tiles
 * 
 * \param x
 *      X position of the tile to move mine from
 * \param y
 *      Y position of the tile to move mine from
*/
void moveMine(u16 x, u16 y)
{
    u16 index = GXYINDEX(x,y);
    if((mineGameData.grid.data[index] & TILEMASK) == TT_MINE)
    {
        mineGameData.grid.data[index] = getNeighboringMineCount(x,y) | TT_COVERED;
        modifyNeighborTileVals(x,y,-1);
        for(int ny=0; ny<mineGameData.grid.height; ny++)
        {
            for(int nx=0; nx<mineGameData.grid.width; nx++)
            {
                u16 newindex = GXYINDEX(nx,ny);
                if(newindex != index && (mineGameData.grid.data[newindex] & TILEMASK) != TT_MINE)
                {
                    mineGameData.grid.data[newindex] = TT_MINE | TT_COVERED | (mineGameData.grid.data[newindex] & TT_FLAGGED);
                    modifyNeighborTileVals(nx,ny,1);
                    nx=MAX_U16; 
                    ny=MAX_U16; //Stops both loops (break only works on one)
                }
            }
        }
    }
}

/**
 * \brief
 *      Check if tile can be uncovered (isn't flagged or uncovered already)
 * \param x
 *      Tile x coordinate
 * \param y
 *      Tile y coordinate
 * \returns
 *      TRUE if tile can be uncovered, FALSE otherwise
*/
bool isTileUncoverPossible(u16 x, u16 y)
{
    if(x < mineGameData.grid.width && y < mineGameData.grid.height)
    {
        u16 index = GXYINDEX(x,y);
        return (mineGameData.grid.data[index] & TT_COVERED && !(mineGameData.grid.data[index] & TT_FLAGGED));
    }
    return FALSE;
}

//Adds tile position to mineGameData.uncoverStack if it's valid
void tryAddingToUncoverStack(u16 x, u16 y)
{
    if(isTileUncoverPossible(x,y))
        mineGameData.uncoverStack[mineGameData.uncoverStackPos++] = PACKCoords(x,y);
    
    if(mineGameData.uncoverStackPos >= TILE_UNCOVER_STACKSIZE)
    {
        SYS_die("Tile uncover stack overflow!");
    }
}

//Removes and returns the last value from mineGameData.uncoverStack and returns it
//MAX_U16 is returned if there's nothing in the stack
u16 popFromTileUncoverStack(void)
{
    if(mineGameData.uncoverStackPos < 1) 
        return MAX_U16;
    return mineGameData.uncoverStack[--mineGameData.uncoverStackPos];
}

//Uncovers a tile without any checks and returns its grid data index
u16 uncoverTile(u16 x, u16 y)
{
    u16 index = GXYINDEX(x,y);
    mineGameData.grid.data[index] &= TILEMASK;
    logic_drawTile(mineGameData.grid.data[index],x,y);
    tileUncoveredCount++;
    return index;
}

/**
 * \brief
 *      Tile uncovering tile function stopping at number tiles using a custom 8-way scanline flood fill algorithm
 * 
 * \param x
 *      X position of the starting tile in the minefield
 * \param y
 *      Y position of the starting tile in the minefield
*/
void startUncoveringTiles(u16 x, u16 y)
{
    tryAddingToUncoverStack(x,y);
    if(mineGameData.uncoverStackPos > 0) //Play sound if tiles are uncovered
    {
        XGM_startPlayPCM(SFX_UNCOVER,1,SOUND_PCM_CH2);
    }
    while(mineGameData.uncoverStackPos > 0)
    {
        u16 pos = popFromTileUncoverStack();
        x = pos >> 8;
        y = pos & 0xFF;
        if(isTileUncoverPossible(x,y))
        {
            u16 index = uncoverTile(x,y);
            if(mineGameData.grid.data[index] == TT_NONE)
            {
                tryAddingToUncoverStack(x-1,y-1);
                tryAddingToUncoverStack(x-1,y+1);
                tryAddingToUncoverStack(x,y-1);
                tryAddingToUncoverStack(x,y+1);
                tryAddingToUncoverStack(x+1,y-1);
                tryAddingToUncoverStack(x+1,y+1);
                u16 nx = x;
                while(isTileUncoverPossible(--nx,y)) //Uncover tiles to the left of the original tile until a boundary or a number tile is reached
                {
                    index = uncoverTile(nx,y);
                    if(mineGameData.grid.data[index] == TT_NONE) //Add remaining diagonally neighboring tiles to the stack if the uncovered tile is empty
                    {
                        tryAddingToUncoverStack(nx-1,y-1);
                        tryAddingToUncoverStack(nx-1,y+1);
                    }
                    else //Number tile - stop loop
                    {
                        break;
                    }
                }
                nx = x;
                while(isTileUncoverPossible(++nx,y)) //Uncover tiles to the right of the original tile until a boundary or a number tile is reached
                {
                    index = uncoverTile(nx,y);
                    if(mineGameData.grid.data[index] == TT_NONE) //Add remaining diagonally neighboring tiles to the stack if the uncovered tile is empty
                    {
                        tryAddingToUncoverStack(nx+1,y-1);
                        tryAddingToUncoverStack(nx+1,y+1);
                    }
                    else //Number tile - stop loop
                    {
                        break;
                    }
                }
            }
        }
    }
}

//Returns TRUE if chording is possible on that tile, FALSE otherwise
bool chordingCheck(u16 x, u16 y)
{
    if(x >= mineGameData.grid.width || y >= mineGameData.grid.height)
        return FALSE;

    u8 tileNum = mineGameData.grid.data[GXYINDEX(x,y)] & TILEMASK;
    if(tileNum > TT_NONE && tileNum <= TT_N8)
    {
        u16 count = 0;
        count += isTileFlagged(x-1,y);
        count += isTileFlagged(x-1,y-1);
        count += isTileFlagged(x-1,y+1);
        count += isTileFlagged(x,y-1);
        count += isTileFlagged(x,y+1);
        count += isTileFlagged(x+1,y);
        count += isTileFlagged(x+1,y-1);
        count += isTileFlagged(x+1,y+1);
        return count == tileNum;
    }
    return FALSE;
}

//Performs chording on a given tile
void chordingTileAction(u16 x, u16 y)
{
    for(s16 dx=-1; dx<=1; dx++)
    {
        for(s16 dy=-1; dy<=1; dy++)
        {
            if(dx == 0 && dy == 0)
                continue;

            u16 nx = x+dx;
            u16 ny = y+dy;
            if(nx < mineGameData.grid.width && ny < mineGameData.grid.height)
            {
                u16 index = GXYINDEX(nx,ny);
                if(!isTileFlagged(nx,ny) && (mineGameData.grid.data[index] & TILEMASK) == TT_MINE)
                {
                    if(setGameCursor(nx,ny))
                    {
                        touchedMine();
                        return;
                    }   
                }
                else
                {
                    startUncoveringTiles(nx,ny);
                }
            }
        }
    }
}

//Update mine count on the HUD display
void updateHUDMineCount(void)
{
    u16 startPosX = (mineGameData.remainingMineCount < 0) ? 4 : 5;
    VDP_clearTextArea(4,2,4,1);
    char buf[8];
    sprintf(buf,"%d",mineGameData.remainingMineCount);
    VDP_drawText(buf,startPosX,2);
}

/**
 * \brief
 *      Show all mine tiles as either mines or flags, used when the game is lost or won
 * \param asFlags
 *      TRUE shows mine tiles as flags (victory), FALSE shows them as mines (defeat)
*/
void showAllMines(bool asFlags)
{
    u8 tileType = TT_MINE;
    if(asFlags)
        tileType |= (TT_FLAGGED|TT_COVERED);
    
    for(u16 x=0; x<mineGameData.grid.width; x++)
    {
        for(u16 y=0; y<mineGameData.grid.height; y++)
        {
            u16 index = GXYINDEX(x,y);
            if((mineGameData.grid.data[index] & TILEMASK) == TT_MINE)
            {
                mineGameData.grid.data[index] = tileType;
                logic_drawTile(tileType,x,y);
            }
        }
    }
}

//Makes the player lose the game
void touchedMine(void)
{
    setLosingPalette2();
    showAllMines(FALSE);
    mineGameData.status = PST_DEAD;
    logic_drawText(NULL);
    XGM_setLoopNumber(0);
    XGM_startPlay(musGameOver);
}

//Checks if all non-mine tiles are uncovered, ends the game with victory and shows all mines as flags if they are
void checkVictory(void)
{
    if(tileUncoveredCount == (mineGameData.grid.width*mineGameData.grid.height)-mineGameData.grid.mineCount)
    {
        showAllMines(TRUE);
        mineGameData.status = PST_VICTORY;
        mineGameData.remainingMineCount = 0;
        updateHUDMineCount();
        logic_drawText(NULL);
        XGM_setLoopNumber(0);
        XGM_stopPlayPCM(SOUND_PCM_CH2);
        XGM_startPlay(musVictory);
    }
}

void logic_drawText(const char* text)
{
    if(!text)
    {
        switch(mineGameData.status)
        {
            case PST_PLAYING:
                text = "PLAYING";
                break;
            case PST_DEAD:
                text = "YOU LOSE";
                break;
            case PST_VICTORY:
                text = "YOU WIN";
                break;
            default:
                text = "";
                break;
        }
    }
    
    char buf[9] = {'\0'};
    memcpy(buf,text,8);
    VDP_clearTextArea(16,2,8,1);
    VDP_drawText(buf,16,2);
}

void logic_drawBorders(void)
{
    VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,borderCTex->vPos),gridXOffset-1,gridYOffset-1);
    for(s16 x=0; x<mineGameData.grid.width; x++)
    {
        VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,borderVTex->vPos),gridXOffset+x,gridYOffset-1);
    }
    for(s16 y=0; y<mineGameData.grid.height; y++)
    {
        VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,borderHTex->vPos),gridXOffset-1,gridYOffset+y);
    }
}

void logic_drawTile(u8 tile, u16 x, u16 y)
{
    u8 palette = (gameCursorX == x && gameCursorY == y) ? PAL2 : PAL1;
    x += gridXOffset;
    y += gridYOffset;
    if(tile & TT_COVERED)
    {
        if(tile & TT_FLAGGED)
        {
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(palette,0,FALSE,FALSE,flaggedTileTex->vPos),x,y);
        }
        else
        {
            VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(palette,0,FALSE,FALSE,coveredTileTex->vPos),x,y);
        }
    }
    else
    {
        tile &= TILEMASK;
        VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(palette,0,FALSE,FALSE,tileTextures[tile]->vPos),x,y);
    }
}

void logic_moveCursor(u16 button)
{
    if(mineGameData.status == PST_DEAD || mineGameData.status == PST_VICTORY)
        return;
    
    u16 oldx = gameCursorX;
    u16 oldy = gameCursorY;
    switch(button)
    {
        case BUTTON_UP:
            if(--gameCursorY < 0)
                gameCursorY = mineGameData.grid.height-1;
            break;
        case BUTTON_DOWN:
            if(++gameCursorY >= mineGameData.grid.height)
                gameCursorY = 0;
            break;
        case BUTTON_LEFT:
            if(--gameCursorX < 0)
                gameCursorX = mineGameData.grid.width-1;
            break;
        case BUTTON_RIGHT:
            if(++gameCursorX >= mineGameData.grid.width)
                gameCursorX = 0;
            break;
        default:
            break;
    }
    logic_drawTile(mineGameData.grid.data[GXYINDEX(oldx,oldy)],oldx,oldy);
    logic_drawTile(mineGameData.grid.data[GXYINDEX(gameCursorX,gameCursorY)],gameCursorX,gameCursorY);
}

void logic_clickTile(bool rightclick, bool isChord)
{
    if(mouse_isEnabled() && !setGameCursor(mouse_getPosition(TRUE).x-gridXOffset,mouse_getPosition(TRUE).y-gridYOffset))
        return;

    u16 index = GXYINDEX(gameCursorX,gameCursorY);
    u8 curTile = mineGameData.grid.data[index];
    
    if(!(curTile & TT_COVERED)) //Chording
    {
        if(!isChord || !chordingCheck(gameCursorX,gameCursorY))
            return;

        chordingTileAction(gameCursorX, gameCursorY);
        checkVictory();
    }
    else if(rightclick)
    {
        if(curTile & TT_FLAGGED)
        {
            mineGameData.grid.data[index] &= ~TT_FLAGGED;
            mineGameData.remainingMineCount++;
        }
        else
        {
            mineGameData.grid.data[index] |= TT_FLAGGED;
            mineGameData.remainingMineCount--;
        }
        updateHUDMineCount();
        logic_drawTile(mineGameData.grid.data[index],gameCursorX,gameCursorY);
    }
    else if(!(curTile & TT_FLAGGED))
    {
        if(mineGameData.status == PST_NOTSTARTED)
        {
            mineGameData.status = PST_PLAYING;
            logic_drawText(NULL);
            moveMine(gameCursorX,gameCursorY);
            curTile = mineGameData.grid.data[index];
        }

        if((curTile & TILEMASK) == TT_MINE)
        {
            touchedMine();
        }
        else if(!(curTile & TT_FLAGGED))
        {
            startUncoveringTiles(gameCursorX,gameCursorY);
            checkVictory();
        }
    }
}

void logic_startGame(void)
{
    setupDifficulty();
    mineGameData.status = PST_NOTSTARTED;
    mineGameData.gameTime = 0;
    tileUncoveredCount = 0;
    gameCursorX = 0;
    gameCursorY = 0;
    u16 width = mineGameData.grid.width;
    u16 height = mineGameData.grid.height;
    gridXOffset = 20-(width>>1);
    gridYOffset = (12-(height>>1))+4;
    u16 gridsize = width*height;
    mineGameData.grid.totalSize = gridsize;
    mineGameData.grid.data = MEM_alloc(gridsize*sizeof(u8));
    if(!mineGameData.grid.data)
    {
        SYS_die("Could not allocate grid data memory!");
    }
    mineGameData.uncoverStack = MEM_alloc(TILE_UNCOVER_STACKSIZE*sizeof(u16));
    if(!mineGameData.uncoverStack)
    {
        SYS_die("Could not allocate tile uncover stack memory!");
    }
    mineGameData.uncoverStackPos = 0;
    memset(mineGameData.grid.data,0,gridsize);
    tileTextures[0] = reserveVImage(&texTile0,TRUE);
    tileTextures[1] = reserveVImage(&texTile1,TRUE);
    tileTextures[2] = reserveVImage(&texTile2,TRUE);
    tileTextures[3] = reserveVImage(&texTile3,TRUE);
    tileTextures[4] = reserveVImage(&texTile4,TRUE);
    tileTextures[5] = reserveVImage(&texTile5,TRUE);
    tileTextures[6] = reserveVImage(&texTile6,TRUE);
    tileTextures[7] = reserveVImage(&texTile7,TRUE);
    tileTextures[8] = reserveVImage(&texTile8,TRUE);
    tileTextures[9] = reserveVImage(&texTileMine,TRUE);
    coveredTileTex = reserveVImage(&texTileCovered,TRUE);
    flaggedTileTex = reserveVImage(&texTileFlagged,TRUE);
    borderHTex = reserveVImage(&texBorderH,TRUE);
    borderVTex = reserveVImage(&texBorderV,TRUE);
    borderCTex = reserveVImage(&texBorderC,TRUE);
    memcpy(&newPalette[16],texTileFlagged.palette->data,sizeof(u16)*texTileFlagged.palette->length);
    memcpy(&newPalette[32],texTileFlagged.palette->data,sizeof(u16)*texTileFlagged.palette->length);
    newPalette[15] = RGB24_TO_VDPCOLOR(0xEE0000);
    if(!mouse_isEnabled())
    {
        newPalette[33] = RGB24_TO_VDPCOLOR(0x00AA00);
        newPalette[34] = RGB24_TO_VDPCOLOR(0x00EE00);
        newPalette[35] = RGB24_TO_VDPCOLOR(0x006600);
        newPalette[43] = RGB24_TO_VDPCOLOR(0x008800);
    }
    initializeGrid();
    updateHUDMineCount();
}

u16 logic_calculateScorePlace(u16* timePtr)
{
    *timePtr = (u16)F32_toInt(mineGameData.gameTime);
    struct TopScore* leaderBoard;
    switch(settings.difficulty)
    {
        case DL_Beginner:
            leaderBoard = lowestTimes.topBeginner;
            break;
        case DL_Intermediate:
            leaderBoard = lowestTimes.topIntermediate;
            break;
        case DL_Expert:
            leaderBoard = lowestTimes.topExpert;
            break;
        default: //Custom difficulty does not have highscores
            return 0;
    }
    for(u16 i=0; i<3; i++)
    {
        if(leaderBoard[i].score > *timePtr)
        {
            return i+1;
        }
    }
    return 0;
}

void logic_stop(void)
{
    ;
}