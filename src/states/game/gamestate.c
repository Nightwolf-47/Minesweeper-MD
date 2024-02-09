#include <gamestate.h>
#include <gamelogic.h>
#include <timer.h>
#include <data.h>
#include <resources.h>
#include <genesis.h>

VidImagePtr gameHudImage;
bool gameEnded = FALSE;

u16 gameJoyDirections = 0; //Set of directional buttons being held
fix32 gameMovementTimer = 0;

const fix32 gameAutoDirTime = FIX32(0.15);
const fix32 gameDirStartTime = FIX32(-0.1);

const fix32 maxGameTime = FIX32(10000)-1;

void drawGame(void)
{
    for(int x=0; x<mineGameData.grid.width; x++)
    {
        for(int y=0; y<mineGameData.grid.height; y++)
        {
            logic_drawTile(mineGameData.grid.data[GXYINDEX(x,y)],x,y);
        }
    }
}

void drawHUD(void)
{
    VDP_drawImageEx(BG_B,gameHudImage->img,TILE_ATTR_FULL(PAL0,0,FALSE,FALSE,gameHudImage->vPos),0,0,FALSE,TRUE);
    logic_drawText(NULL);
}

void updateHUDTime(void)
{
    char buf[5];
    sprintf(buf,"%04u",min((u16)fix32ToInt(mineGameData.gameTime),9999));
    VDP_drawText(buf,32,2);
}

void gamestate_init(void)
{
    gameJoyDirections = 0;
    gameMovementTimer = 0;
    gameEnded = FALSE;
    gameHudImage = reserveVImage(&texGameHUD,FALSE);
    memcpy(newPalette,texGameHUD.palette->data,texGameHUD.palette->length*sizeof(u16));
    logic_startGame();
    logic_drawBorders();
    drawHUD();
    drawGame();
}

void gamestate_update(fix32 dt)
{
    if(gameEnded)
    {
        gameMovementTimer = 0;
        gameEnded = FALSE;
        if(mineGameData.status == PST_VICTORY)
        {
            u16 leaderBoardTime;
            u16 place = logic_calculateScorePlace(&leaderBoardTime);
            if(place > 0)
            {
                lowestTimes.lastScore = leaderBoardTime | (place << 16);
                changeState(ST_SCORESTATE);
            }
            else
            {
                changeState(ST_MENUSTATE);
            }
        }
        else
        {
            changeState(ST_MENUSTATE);
        }
        
        mineGameData.status = PST_NOTSTARTED;
        return;
    }
    if(mineGameData.status == PST_PLAYING)
    {
        mineGameData.gameTime += dt;
        mineGameData.gameTime = min(mineGameData.gameTime, maxGameTime);
        updateHUDTime();
    }
    if(mineGameData.status == PST_NOTSTARTED || mineGameData.status == PST_PLAYING)
    {
        //Automatic cursor movement stuff
        if(gameJoyDirections != 0)
        {
            gameMovementTimer += dt;
            if(gameMovementTimer >= gameAutoDirTime)
            {
                gameMovementTimer = 0;
                s16 movementDir[2] = {
                    -((gameJoyDirections & BUTTON_LEFT) != 0) + ((gameJoyDirections & BUTTON_RIGHT) != 0),
                    -((gameJoyDirections & BUTTON_UP) != 0) + ((gameJoyDirections & BUTTON_DOWN) != 0)
                };
                if(movementDir[0] < 0)
                    logic_moveCursor(BUTTON_LEFT);
                else if(movementDir[0] > 0)
                    logic_moveCursor(BUTTON_RIGHT);
                
                if(movementDir[1] < 0)
                    logic_moveCursor(BUTTON_UP);
                else if(movementDir[1] > 0)
                    logic_moveCursor(BUTTON_DOWN);
            }
        }
    }
}

void gamestate_joyevent(u16 joy, u16 changed, u16 state)
{
    if(joy==JOY_1)
    {
        if(state & changed)
        {
            if(changed & BUTTON_DIR && (gameJoyDirections == 0 || gameMovementTimer < 0))
            {
                logic_moveCursor(changed);
                gameMovementTimer = gameDirStartTime;
            }
            else if(changed == BUTTON_START)
            {
                gameEnded = TRUE;
            }
            else if(changed == BUTTON_A || changed == BUTTON_B || changed == BUTTON_C)
            {
                u16 leftButtonsPressed = state & (BUTTON_A|BUTTON_B);
                u16 middleButtonPressed = state & BUTTON_B;
                u16 rightButtonPressed = state & BUTTON_C;
                switch(mineGameData.status)
                {
                    case PST_NOTSTARTED:
                    case PST_PLAYING:
                        logic_clickTile(rightButtonPressed, (leftButtonsPressed && rightButtonPressed) || middleButtonPressed);
                        break;
                    case PST_DEAD:
                    case PST_VICTORY:
                        gameEnded = TRUE;
                        break;
                    default:
                        break;
                }  
            }
        }
        gameJoyDirections = state & BUTTON_DIR;
    }
}

void gamestate_stop(void)
{
    if(mineGameData.grid.data)
    {
        MEM_free(mineGameData.grid.data);
        mineGameData.grid.data = NULL;
    }
    if(mineGameData.uncoverStack)
    {
        MEM_free(mineGameData.uncoverStack);
        mineGameData.uncoverStack = NULL;
    }
    logic_stop();
}