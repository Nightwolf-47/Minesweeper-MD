#include <data.h>
#include <save.h>
#include <mouse.h>

void joyEventHandler(u16 joy, u16 changed, u16 state)
{
    if(mouse_isEnabled())
        state &= ~BUTTON_DIR;
    
    if(states[currentState].joyevent && !isChangingState)
        states[currentState].joyevent(joy,changed,state);
}

int main(bool hard)
{
    mouse_stop();
    if(JOY_getPortType(PORT_1) == PORT_TYPE_MOUSE)
        mouse_init();  
    SPR_init();
    JOY_setEventHandler(&joyEventHandler);
    setupStates();
    data_initsfx();
    if(hard)
    {
        initData();
        loadSRAM(); //Load settings if they exist
    }
    initState(ST_MENUSTATE);
    fix32 dt = 1;
    fix32 totalTime = getTimeAsFix32(0);
    if(hard)
    {
        dt = totalTime; //DeltaTime is only equal to the total time right after a hard reset
    }
    while(TRUE)
    {
        random();
        if(states[currentState].update)
            states[currentState].update(dt);
        if(mouse_isEnabled())
            mouse_update();
        SPR_update();
        SYS_doVBlankProcess();
        fix32 newTime = getTimeAsFix32(0);
        dt = newTime - totalTime;
        totalTime = newTime;
    }
    return (0);
}
