#ifndef SCREENS_H
#define SCREENS_H

extern uint8_t inputField[20];

/// @brief Save screen to memory for quick recall - returns true if successful
bool saveScreen();

/// @brief Restore screen - returns true if successful
bool restoreScreen();

/// @brief Reset screen and draw border
void resetScreenWithBorder();

/// @brief Reset entire screen
void resetScreenNoBorder();

/// @brief Shows information about the game
void showHelpScreen();

/// @brief Action called in Welcome Screen to check if a server name is stored in an app key
void welcomeActionVerifyServerDetails();

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName();

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen();

/// @brief Shows a screen to select a table to join
void showTableSelectionScreen();

/// @brief Shows main game play screen (table and cards)
void showGameScreen();

/// @brief shows in-game menu
void showInGameMenuScreen();

/// @brief Allow a local player to modify their name
void showPlayerNameScreen(uint8_t p);

/// @brief Choose multiple local players to play
void showPlayerGroupScreen();

/// @brief Draw the title
void drawLogo(uint8_t x, uint8_t y);

#endif /*SCREENS_H*/