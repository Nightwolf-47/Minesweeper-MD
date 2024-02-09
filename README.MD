# Minesweeper MD

Mega Drive port of Minesweeper with features such as: 
- Big grid sizes (up to 38x22)  
- Leaderboards with 3 best times per difficulty  
- Chording
- Mega Mouse support  

## Controls

### In main menu
- A/B/Right - move value forward
- C/Left - move value backwards
- A/B/C - select option (Start game & Reset Settings)
### In-game
- A/B - Tile uncovering  
- C - Tile flagging  
- B - Chording  
- START - Return to menu
### Mega Mouse control mapping
- Left click -> A
- Middle click -> B
- Right click -> C  

## Options
- Difficulty - Set the game's difficulty
- Reset settings - Reset all settings, but not leaderboard data  
#### Custom difficulty only:
- Grid width - Set in-game grid width (5-38)  
- Grid height - Set in-game grid height (5-22)  
- Mine count - Set the mine count  

### Difficulty settings
- Easy (9x9 grid with 10 mines)  
- Normal (16x16 grid with 40 mines)  
- Expert (30x16 grid with 99 mines)  
- Custom (User-specified grid width, height and mine count)  

## Credits  
**Stephane Dallongeville** - [SGDK](https://github.com/Stephane-D/sgdk), a development kit used to compile this game.  
**DrPetter** - SFXR, a tool used to make sounds for this game.  
**TildeArrow** - [Furnace](https://github.com/tildearrow/furnace), a chiptune tracker used to make the victory and game over jingles.  

## SGDK information
The game is compiled with SGDK 2.00, but SGDK 1.90 should also work as it was the version used for most of the game's development.  

## License
This game is licensed under the [MIT License](https://github.com/Nightwolf-47/Minesweeper-MD/blob/main/LICENSE).
