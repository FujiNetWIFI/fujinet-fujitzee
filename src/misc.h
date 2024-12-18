#ifndef MISC_H
#define MISC_H

#include <joystick.h>
#include <conio.h>
#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include <stdbool.h>
#include <stdint.h>

// FujiNet AppKey settings. These should not be changed
#define AK_LOBBY_CREATOR_ID 1     // FUJINET Lobby
#define AK_LOBBY_APP_ID 1         // Lobby Enabled Game
#define AK_LOBBY_KEY_USERNAME 0   // Lobby Username key
#define AK_LOBBY_KEY_SERVER 3     // Fujzee registered as Lobby appkey 3 at https://github.com/FujiNetWIFI/fujinet-firmware/wiki/SIO-Command-$DC-Open-App-Key

// Fujitzee
#define AK_CREATOR_ID 0xE41C      // Eric Carr's creator id
#define AK_APP_ID 3               // Fujzee App ID
#define AK_KEY_PREFS 0            // Preferences

#define PLAYER_MAX 12

#define FUJITZEE_SCORE 14

typedef struct {
  char table    [9];
  char name     [21];
  char players  [6];
} Table;

typedef struct {
  char name[9];
  uint8_t alias;
  int16_t scores[16];
} Player;

typedef struct {
  uint8_t count;
  Table table[10];
} Tables;

typedef struct {
  uint8_t playerCount;
  char serverName [21];
  char prompt     [41];
  uint8_t round;
  uint8_t rollsLeft;
  int8_t activePlayer;
  uint8_t moveTime;
  uint8_t viewing;
  char dice     [6];
  char keepRoll [6];
  int8_t validScores[15];
  Player players[PLAYER_MAX];
} Game;

typedef union {
  uint8_t firstByte;
  Game game;
  Tables tables;
} ClientState;

extern ClientState clientState;

typedef struct {
  char query[50]; //?table=12345678&pov=12345678&player=12345678
  uint8_t index;
} LocalPlayerState;

typedef struct {
  
  // Internal game state
  uint8_t rollFrames;

  uint8_t prevRollsLeft;
  uint8_t prevPlayerCount;
  uint8_t prevRound;

  uint8_t apiCallWait;

  int8_t prevActivePlayer;
  
  bool playerMadeMove;

  bool countdownStarted;
  bool waitingOnEndGameContinue;
  bool drawBoard;
  bool isViewing[PLAYER_MAX];

  int8_t currentLocalPlayer;
  bool localPlayerIsActive;
  LocalPlayerState localPlayer[4];
  bool renderedScore[16*6];
  bool inGame;
  char prevKept[6];
  char prevDice[6];
  
} GameState;

typedef struct {
  unsigned char key;
  bool trigger;
  int8_t dirX;
  int8_t dirY;
} InputStruct;


typedef struct {
  char name[9];
} LocalPlayer;

typedef struct {
  bool seenHelp;
  uint8_t color;
  uint8_t debugFlag; // 0xFF to use localhost instead of server
  uint8_t localPlayerCount;
  LocalPlayer localPlayer[4];
  uint8_t disableSound;
  bool hasPlayed;
} PrefsStruct;


extern uint16_t maxJifs;
extern char tempBuffer[128];
extern char serverEndpoint[50];
extern char localServer[];

extern GameState state;
extern InputStruct input;
extern PrefsStruct prefs;

// Common local scope temp variables
extern unsigned char h, i, j, k, x, y;

void pause(unsigned char frames);
void clearCommonInput();
void readCommonInput();
void loadPrefs();
void savePrefs();


/// @brief Helper method to write to an appkey
void write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id,  uint16_t count, char *data);

/// @brief Helper method to read from an appkey.
/// NULL will be appended to data in case this is a string, though the length returned will not consider the NULL.
uint16_t read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char* destination);

#endif /* MISC_H */