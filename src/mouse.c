#include <mouse.h>
#include <genesis.h>
#include <resources.h>

static Sprite* mouseSprite;

static bool mouseIsEnabled = FALSE;

static s16 mousePosX = 0;
static s16 mousePosY = 0;

static inline u16 mouseConvCoord(s16 coord)
{
    return (u16)coord >> 2;
}

void mouse_init(void)
{
    if(mouseIsEnabled)
        return;
    JOY_setSupport(PORT_1, JOY_SUPPORT_MOUSE);
    mouseSprite = SPR_addSpriteSafe(&sprCursor,0,0,TILE_ATTR(PAL1,1,FALSE,FALSE));
    SPR_setDepth(mouseSprite,SPR_MIN_DEPTH);
    mouseIsEnabled = TRUE;
}

void mouse_update(void)
{
    mousePosX = clamp(JOY_readJoypadX(JOY_1), 0, 1275);
    JOY_writeJoypadX(JOY_1,mousePosX);
    mousePosY = clamp(JOY_readJoypadY(JOY_1), 0, 891);
    JOY_writeJoypadY(JOY_1,mousePosY);
    SPR_setPosition(mouseSprite,mouseConvCoord(mousePosX),mouseConvCoord(mousePosY));
}

void mouse_changeState(enum States newState)
{
    if(mouseSprite)
        SPR_releaseSprite(mouseSprite);
    mouseSprite = SPR_addSpriteSafe(&sprCursor,mouseConvCoord(mousePosX),mouseConvCoord(mousePosY),TILE_ATTR(PAL1,1,FALSE,FALSE));
    SPR_setDepth(mouseSprite,SPR_MIN_DEPTH);
}

MousePosition mouse_getPosition(bool inTiles)
{
    MousePosition mpos;
    mpos.x = inTiles ? (mouseConvCoord(mousePosX)>>3) : mouseConvCoord(mousePosX);
    mpos.y = inTiles ? (mouseConvCoord(mousePosY)>>3) : mouseConvCoord(mousePosY);
    return mpos;
}

bool mouse_isEnabled(void)
{
    return mouseIsEnabled;
}

void mouse_stop(void)
{
    if(mouseSprite)
    {
        SPR_releaseSprite(mouseSprite);
        mouseSprite = NULL;
    }
    mouseIsEnabled = FALSE;
}