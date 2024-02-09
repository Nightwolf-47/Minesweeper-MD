#include <save.h>
#include <data.h>
#include <genesis.h>

/*
saveVersion only changes when any of the saved data structures do, in which case it's as below:

Prerelease versions - 0x0PVR
    P - phase (A-alpha, B-beta, C-prerelease, D-release candidate)
    V - version number
    R - revision

Release versions - 0xXYZR
    version X.Y.Z, revision R
    (X needs to be at least 1)
*/
const u16 saveVersion = 0x0B40;

void loadSRAM(void)
{
    SRAM_enableRO();
    u16 curSaveVersion = SRAM_readWord(0); //Validation check
    if(curSaveVersion==saveVersion)
    {
        u8* membuf = (u8*)&settings;
        u16 sOffset = SRAM_START_POS;
        for(u16 i=0; i<sizeof(settings); i++)
        {
            membuf[i] = SRAM_readByte(i+sOffset);
        }
        sOffset += sizeof(settings);
        membuf = (u8*)&lowestTimes;
        for(u16 i=0; i<sizeof(lowestTimes); i++)
        {
            membuf[i] = SRAM_readByte(i+sOffset);
        }
    }
    SRAM_disable();
}

void saveSRAM(void)
{
    SRAM_enable();
    SRAM_writeWord(0,saveVersion);
    u8* membuf = (u8*)&settings;
    u16 sOffset = SRAM_START_POS;
    for(u16 i=0; i<sizeof(settings); i++)
    {
        SRAM_writeByte(i+sOffset,membuf[i]);
    }
    sOffset += sizeof(settings);
    lowestTimes.lastScore = MAX_U32;
    membuf = (u8*)&lowestTimes;
    for(u16 i=0; i<sizeof(lowestTimes); i++)
    {
        SRAM_writeByte(i+sOffset,membuf[i]);
    }
    SRAM_disable();
}

void clearSRAM(void)
{
    SRAM_enable();
    SRAM_writeWord(0,0); //saveVersion == 0 means there's no valid save data
    SRAM_disable();
}