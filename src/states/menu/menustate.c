#include <menustate.h>
#include <data.h>
#include <genesis.h>
#include <gamelogic.h>
#include <save.h>
#include <resources.h>
#include <mouse.h>

const u16 menuOptionStartY = 4;

const u16 menuLeaderTextStartY = 23;

s16 menuOptionPos = 0;

fix32 menuPressTimer = -1;
u16 menuButtonPressed = 0;
u16 movedMenuButtonEntries = 0;

const fix32 menuAutoTime = FIX32(0.2);
const fix32 menuPressStartTime = FIX32(-0.1);

u16 menuMaxMinecount = MAXGRIDSIZE-1;

bool menuButtonsLocked = FALSE;

const char* difficultyStrings[4] = {
    "Beginner",
    "Intermediate",
    "Expert",
    "Custom"
};

const char* optionStrings[6] = {
    "Start the game",
    "Difficulty",
    "Width",
    "Height",
    "Mine count",
    "Reset Settings"
};

VidImagePtr menuBackgroundImg;
Image* texMenuBGUnpacked;

//Adjusts the mine count setting so that it's lower than the grid size
void adjustMenuMineCount(void)
{
    menuMaxMinecount = min(settings.width*settings.height, MAXGRIDSIZE)-1;
    settings.mineCount = min(settings.mineCount, menuMaxMinecount);
}

//Draw menu selection cursor in the correct position
void drawMenuCursor(void)
{
    s16 realPos = (settings.difficulty == DL_Custom) ? menuOptionPos : min(menuOptionPos,2);
    VDP_clearTextArea(3,menuOptionStartY,1,12);
    VDP_clearTextArea(36,menuOptionStartY,1,12);
    VDP_drawText(">",3,menuOptionStartY+(realPos*2));
    VDP_drawText("<",36,menuOptionStartY+(realPos*2));
}

//Draw values of menu settings, used when drawing the entire menu and when a value changes
void drawMenuValues(void)
{
    VDP_setTextPalette(PAL2);
    char buf[24] = {'\0'};
    const char* difficultyStr = difficultyStrings[settings.difficulty];
    VDP_clearTextArea(20,menuOptionStartY,15,12);
    
    VDP_drawText(difficultyStr,GETRIGHTX(difficultyStr,5),menuOptionStartY+2);
    if(settings.difficulty == DL_Custom)
    {
        sprintf(buf,"%u",settings.width);
        VDP_drawText(buf,GETRIGHTX(buf,5),menuOptionStartY+4);
        sprintf(buf,"%u",settings.height);
        VDP_drawText(buf,GETRIGHTX(buf,5),menuOptionStartY+6);
        sprintf(buf,"%u",settings.mineCount);
        VDP_drawText(buf,GETRIGHTX(buf,5),menuOptionStartY+8);
    }
    VDP_setTextPalette(PAL0);
}

//Draw leaderboard scores
void drawLeaderboard(void)
{
    VDP_setTextPalette(PAL2);
    char buf[16];
    for(u16 i=0; i<3; i++)
    {
        if(strlen(lowestTimes.topBeginner[i].name) > 0)
        {
            sprintf(buf,"%6s %4u",lowestTimes.topBeginner[i].name,lowestTimes.topBeginner[i].score);
            VDP_drawText(buf,3,menuLeaderTextStartY+i);
        }
        else
        {
            VDP_drawText(" --------- ",3,menuLeaderTextStartY+i);
        }
    }
    for(u16 i=0; i<3; i++)
    {
        if(strlen(lowestTimes.topIntermediate[i].name) > 0)
        {
            sprintf(buf,"%6s %4u",lowestTimes.topIntermediate[i].name,lowestTimes.topIntermediate[i].score);
            VDP_drawText(buf,15,menuLeaderTextStartY+i);
        }
        else
        {
            VDP_drawText(" --------- ",15,menuLeaderTextStartY+i);
        }
    }
    for(u16 i=0; i<3; i++)
    {
        if(strlen(lowestTimes.topExpert[i].name) > 0)
        {
            sprintf(buf,"%6s %4u",lowestTimes.topExpert[i].name,lowestTimes.topExpert[i].score);
            VDP_drawText(buf,27,menuLeaderTextStartY+i);
        }
        else
        {
            VDP_drawText(" --------- ",27,menuLeaderTextStartY+i);
        }
    }
    VDP_setTextPalette(PAL0);
}

/// @brief Draw the entire menu including the leaderboard
/// @param drawImage if TRUE, draw the background image and menu text, otherwise only draw menu text
void drawMenu(bool drawImage)
{
    VDP_clearPlane(BG_A,TRUE);
    if(drawImage)
        VDP_drawImageEx(BG_B,menuBackgroundImg->img,TILE_ATTR_FULL(PAL1,0,FALSE,FALSE,menuBackgroundImg->vPos),1,menuOptionStartY-3,FALSE,TRUE);
    for(s16 i=0; i<6; i++)
    {
        if(i<2 || settings.difficulty == DL_Custom)
            VDP_drawText(optionStrings[i],5,menuOptionStartY+(i*2));
        else if(i==5 && settings.difficulty != DL_Custom)
            VDP_drawText(optionStrings[i],5,menuOptionStartY+4);
    }
    VDP_drawText(versionStr,3,menuOptionStartY+(6*2));
    drawMenuValues();
    drawMenuCursor();
    drawLeaderboard();
}

/// @brief Change menu option and move cursor
/// @param back if TRUE, moves cursor up, otherwise moves down
void changeMenuOptionPos(bool back)
{
    if(back)
    {
        if(menuOptionPos == 5 && settings.difficulty != DL_Custom)
        {
            menuOptionPos = 1;
        }
        else
        {
            menuOptionPos--;
            if(menuOptionPos < 0)
            {
                menuOptionPos = 5;
            }
        }
    }
    else
    {
        if(menuOptionPos == 1 && settings.difficulty != DL_Custom)
        {
            menuOptionPos = 5;
        }
        else
        {
            menuOptionPos++;
        }
    }
    menuOptionPos %= 6;
    drawMenuCursor();
}

/// @brief Change current menu value or selects an option
/// @param button SGDK button constant
void changeMenuValue(u16 button)
{
    bool isBack = button & (BUTTON_LEFT|BUTTON_C);
    switch(menuOptionPos)
    {
        case 0:
            if(button & BUTTON_BTN)
            {
                menuButtonsLocked = TRUE;
            }
            break;
        case 1:
            if(isBack)
            {
                if(settings.difficulty == DL_Beginner)
                    settings.difficulty = DL_Custom;
                else
                    settings.difficulty--;
            }
            else
            {
                if(++settings.difficulty > DL_Custom)
                    settings.difficulty = DL_Beginner;
            }
            drawMenu(FALSE);
            break;
        case 2:
            if(isBack)
            {
                if(--settings.width < MIN_WIDTH_HEIGHT)
                    settings.width = 38;
            }
            else
            {
                if(++settings.width > 38)
                    settings.width = MIN_WIDTH_HEIGHT;
            }
            adjustMenuMineCount();
            drawMenuValues();
            break;
        case 3:
            if(isBack)
            {
                if(--settings.height < MIN_WIDTH_HEIGHT)
                    settings.height = 22;
            }
            else
            {
                if(++settings.height > 22)
                    settings.height = MIN_WIDTH_HEIGHT;
            }
            adjustMenuMineCount();
            drawMenuValues();
            break;
        case 4:
            if(isBack)
            {
                if(--settings.mineCount < 1)
                    settings.mineCount = menuMaxMinecount;
            }
            else
            {
                if(++settings.mineCount > menuMaxMinecount)
                    settings.mineCount = 1;
            }
            drawMenuValues();
            break;
        case 5:
            initSettings();
            saveSRAM();
            drawMenu(FALSE);
            break;
        default:
            return;
            break;
    }
    XGM_startPlayPCM(SFX_CLICK,1,SOUND_PCM_CH2);
}

void menustate_init(void)
{
    adjustMenuMineCount();
    texMenuBGUnpacked = unpackImage(&texMenuBG,NULL);
    menuBackgroundImg = reserveVImage(texMenuBGUnpacked,FALSE);
    memcpy(&newPalette[16],texMenuBG.palette->data,texMenuBG.palette->length*sizeof(u16));
    newPalette[15] = RGB24_TO_VDPCOLOR(0x000000);
    newPalette[47] = RGB24_TO_VDPCOLOR(0x000088);
    menuButtonsLocked = FALSE;
    menuPressTimer = -1;
    menuButtonPressed = 0;
    movedMenuButtonEntries = 0;
    drawMenu(TRUE);
}

void menustate_update(fix32 dt)
{
    if(menuButtonsLocked)
    {
        saveSRAM();
        changeState(ST_GAMESTATE);
        menuButtonsLocked = FALSE;
    }
    else if(menuButtonPressed > 0)
    {
        menuPressTimer += dt;
        if(menuPressTimer >= menuAutoTime)
        {
            menuPressTimer = 0;
            changeMenuValue(menuButtonPressed);
            movedMenuButtonEntries++;
            if(menuOptionPos == 4 && movedMenuButtonEntries > 50)
            {
                menuPressTimer = (menuAutoTime*3) >> 1;
                XGM_stopPlayPCM(SOUND_PCM_CH2);
            }
            else if(menuOptionPos >= 2 && menuOptionPos <= 4 && movedMenuButtonEntries > 10)
            {
                menuPressTimer = menuAutoTime >> 1;
            }
        }
    }
    else if(mouse_isEnabled())
    {
        MousePosition mpos = mouse_getPosition(TRUE);
        if(mpos.x >= 2 && mpos.x <= 38 && mpos.y >= menuOptionStartY && mpos.y < menuOptionStartY+12)
        {
            menuOptionPos = (mpos.y - menuOptionStartY)>>1;
            if(menuOptionPos > 1 && settings.difficulty != DL_Custom)
                menuOptionPos = 5;
            drawMenuCursor();
        }
    }
}

void menustate_joyevent(u16 joy, u16 changed, u16 state)
{
    if(joy==JOY_1 && !menuButtonsLocked)
    {
        if(state & changed)
        {
            switch(changed)
            {
                case BUTTON_DOWN:
                    changeMenuOptionPos(FALSE);
                    break;
                case BUTTON_UP:
                    changeMenuOptionPos(TRUE);
                    break;
                case BUTTON_LEFT:
                case BUTTON_RIGHT:
                case BUTTON_A:
                case BUTTON_B:
                case BUTTON_C:
                    menuPressTimer = menuPressStartTime;
                    menuButtonPressed = changed;
                    movedMenuButtonEntries = 0;
                    changeMenuValue(changed);
                    break;
            }
        }
        else
        {
            if(!(state & menuButtonPressed))
            {
                menuPressTimer = menuPressStartTime;
                movedMenuButtonEntries = 0;
            }

            if(state & (BUTTON_LEFT|BUTTON_C))
            {
                menuButtonPressed = state & (BUTTON_LEFT|BUTTON_C);
            }
            else if(state & (BUTTON_RIGHT|BUTTON_A|BUTTON_B))
            {
                menuButtonPressed = state & (BUTTON_RIGHT|BUTTON_A|BUTTON_B);
            }
            else
            {
                menuPressTimer = -1;
                menuButtonPressed = 0;
                movedMenuButtonEntries = 0;
            }
        }
    }
}

void menustate_stop(void)
{
    if(texMenuBGUnpacked)
    {
        MEM_free(texMenuBGUnpacked);
        texMenuBGUnpacked = NULL;
    }
}