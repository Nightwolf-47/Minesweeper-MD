#ifndef SAVE_H_INCLUDED
#define SAVE_H_INCLUDED

#define SRAM_START_POS 2

//Load data from SRAM (persistent memory), should only be called on first startup/hard reset
void loadSRAM(void);

//Save data to SRAM (persistent memory)
void saveSRAM(void);

//Clear saved SRAM data
void clearSRAM(void);

#endif