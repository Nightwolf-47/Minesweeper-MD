#include <scorestate.h>
#include <data.h>
#include <genesis.h>
#include <resources.h>
#include <save.h>
#include <mouse.h>

static u16 lastScoreNum = 0;
static u16 lastScore = 0;
static bool isValidLastScore = FALSE;

static const u16 scoreMenuStartX = 1;
static const u16 scoreMenuStartY = 5;

static u16 selectedKeyX = 0;
static u16 selectedKeyY = 0;

static u8 selectedCharIndex = 0;
static char newScoreName[SCORE_NAME_LEN] = {'\0'};

static VidImagePtr scoreBgImg;
static Sprite* keySelector = NULL;

static char scoreMenuKeys[] = "1234567890QWERTYUIOPASDFGHJKL ZXCVBNM~\x7F ";

static char emptyNameString[] = "      ";

//Draw the top message (about the current leaderboard place and time)
static void drawNewScoreTime(void)
{
    char buf[38];
    const char* stndrdStr = (lastScoreNum == 1) ? "st" : ((lastScoreNum == 2) ? "nd" : "rd");
    sprintf(buf,"You got %u%s place! [time: %u s]",lastScoreNum,stndrdStr,min(lastScore,9999));
    VDP_drawText(buf,GETCENTERX(buf),scoreMenuStartY+1);
}

//Convert key cursor position to a position in 8x8 VDP tiles
static void convertKeyPosToTilePos(u16 kx, u16 ky, u16* tx, u16* ty)
{
    *tx = scoreMenuStartX+5+(kx*3);
    *ty = scoreMenuStartY+7+(ky*3);
}

//Move key cursor to specific position
static void moveKeyCursor(s16 x, s16 y)
{
    x = (x+100) % 10; //The addition is here to make negative x values properly wrap around
    y &= 3;
    selectedKeyX = x;
    selectedKeyY = y;
    u16 tx,ty;
    convertKeyPosToTilePos(x,y,&tx,&ty);
    SPR_setPosition(keySelector,(tx-1)<<3,(ty-1)<<3);
}

//Move character selector by 'move' and refresh input text
static void moveNameCharIndex(s8 move)
{
    selectedCharIndex += move + (SCORE_NAME_LEN-1); //The SCORE_NAME_LEN-1 addition is here to make negative charindex values properly wrap around
    selectedCharIndex %= (SCORE_NAME_LEN-1);
    VDP_clearTileMapRect(BG_B,scoreMenuStartX+16,scoreMenuStartY+4,6,1);
    VDP_clearText(scoreMenuStartX+16,scoreMenuStartY+4,6);
    VDP_setTileMapXY(BG_B,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,4),scoreMenuStartX+16+selectedCharIndex,scoreMenuStartY+4);
    VDP_setTextPalette(PAL2);
    VDP_drawText(newScoreName,scoreMenuStartX+16,scoreMenuStartY+4);
    VDP_setTextPalette(PAL0);
}

//Draw the entire menu
static void drawNewScoreMenu(void)
{
    VDP_drawImageEx(BG_B,scoreBgImg->img,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,scoreBgImg->vPos),scoreMenuStartX,scoreMenuStartY,FALSE,TRUE);
    VDP_setTextPalette(PAL2);
    for(u16 x=0; x<10; x++)
    {
        for(u16 y=0; y<4; y++)
        {
            u16 index = y*10+x;
            char keyStr[2] = {scoreMenuKeys[index],'\0'};
            if(scoreMenuKeys[index] != ' ')
            {
                u16 tx,ty;
                convertKeyPosToTilePos(x,y,&tx,&ty);
                VDP_drawText(keyStr,tx,ty);
            }
        }
    }
    VDP_setTextPalette(PAL0);
    drawNewScoreTime();
}

//Save the score to leaderboard and return to menu
static void finishScoreMenu(void)
{
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
            return;
    }
    if(strcmp(newScoreName,emptyNameString) == 0)
        memcpy(newScoreName,"PLAYER",sizeof(newScoreName));
    
    for(u16 i=2; i>=lastScoreNum; i--)
    {
        memcpy(&leaderBoard[i],&leaderBoard[i-1],sizeof(leaderBoard[i]));
    }
    memcpy(leaderBoard[lastScoreNum-1].name,newScoreName,sizeof(newScoreName));
    leaderBoard[lastScoreNum-1].score = lastScore;
    saveSRAM();
    changeState(ST_MENUSTATE);
}

//Handles button selection, either writes a character or does a special action
static void keyButtonSelection(void)
{
    u16 index = selectedKeyY*10+selectedKeyX;
    switch(index)
    {
        case 29:
            newScoreName[selectedCharIndex] = ' ';
            moveNameCharIndex(0);
            break;
        case 37:
            moveNameCharIndex(-1);
            break;
        case 38:
            moveNameCharIndex(1);
            break;
        case 39:
            finishScoreMenu();
            break;
        default:
            newScoreName[selectedCharIndex] = scoreMenuKeys[index];
            moveNameCharIndex(1);
            break;
    }
}

void scorestate_init(void)
{
    selectedKeyX = 0;
    selectedKeyY = 0;
    selectedCharIndex = 0;
    memset(newScoreName,' ',sizeof(newScoreName));
    newScoreName[SCORE_NAME_LEN-1] = '\0';
    if(lowestTimes.lastScore != MAX_U32)
    {
        isValidLastScore = TRUE;
        lastScore = lowestTimes.lastScore & 0xFFFF;
        lastScoreNum = lowestTimes.lastScore >> 16;
        lowestTimes.lastScore = MAX_U32;
        scoreBgImg = reserveVImage(&texScoreInputBG,FALSE);
        newPalette[15] = RGB24_TO_VDPCOLOR(0x000000);
        newPalette[47] = RGB24_TO_VDPCOLOR(0xFFFFFF);
        u16 tx,ty;
        convertKeyPosToTilePos(0,0,&tx,&ty);
        keySelector = SPR_addSpriteSafe(&sprKeySel,(tx-1)<<3,(ty-1)<<3,TILE_ATTR(PAL1,0,FALSE,FALSE));
        memcpy(&newPalette[16],texScoreInputBG.palette->data,texScoreInputBG.palette->length*sizeof(u16));
        drawNewScoreMenu();
        moveNameCharIndex(0);
    }
    else
    {
        keySelector = NULL;
        isValidLastScore = FALSE;
    }
}

void scorestate_update(fix32 dt)
{
    if(!isValidLastScore)
    {
        if(!isChangingState)
            changeState(ST_MENUSTATE);

        return;
    }

    if(mouse_isEnabled())
    {
        //Convert mouse position to key cursor position
        MousePosition mpos = mouse_getPosition(FALSE);
        mpos.x = max(mpos.x - 32 - (scoreMenuStartX<<3), 0) / 24;
        mpos.y = max(mpos.y - 48 - (scoreMenuStartY<<3), 0) / 24;
        mpos.x = min(mpos.x,9);
        mpos.y = min(mpos.y,3);
        moveKeyCursor(mpos.x,mpos.y);
    }
}

void scorestate_joyevent(u16 joy, u16 changed, u16 state)
{
    if(state & changed)
    {
        s8 dx = 0;
        s8 dy = 0;
        switch(changed)
        {
            case BUTTON_LEFT:
                dx = -1;
                break;
            case BUTTON_RIGHT:
                dx = 1;
                break;
            case BUTTON_UP:
                dy = -1;
                break;
            case BUTTON_DOWN:
                dy = 1;
                break;
            case BUTTON_A:
            case BUTTON_B:
            case BUTTON_C:
                keyButtonSelection();
                break;
            case BUTTON_START:
                break;
        }
        moveKeyCursor(selectedKeyX+dx,selectedKeyY+dy);
    }
}

void scorestate_stop(void)
{
    if(keySelector)
    {
        SPR_releaseSprite(keySelector);
        keySelector = NULL;
    }
}