#include <stdlib.h>
#include "stateclient.h"
#include "screens.h"
#include "misc.h"
#include "gamelogic.h"

#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include "platform-specific/util.h"
#include "platform-specific/sound.h"
#include <string.h>

#define PLAYER_NAME_MAX 8
#define PLAYER_BOX_TOP 13
#define REMOVE_PLAYER_KEY '/'
#define INGAME_MENU_X WIDTH/2-8

char query[20] = "";
bool inBorderedScreen=false, prevBorderedScreen=false;


void saveScreen() {
  prevBorderedScreen=inBorderedScreen;
  saveScreenBuffer();
}

void restoreScreen() {
  inBorderedScreen=prevBorderedScreen;
  restoreScreenBuffer();
  //TODO - handle lack of save/restore-
}

void resetScreenNoBorder() {
  inBorderedScreen=false;
  resetScreen(inBorderedScreen);
}

/// @brief Convenience function to reset screen and draw border
void resetScreenWithBorder() {
  
  resetScreen(inBorderedScreen);
  
  if (!inBorderedScreen) {
    // Draw dice corners
    drawDie(0,0,1, 0,0);
    //drawDie(0,HEIGHT/2-2,"5", 0);
    drawDie(0,HEIGHT-3,3, 0,0);

    drawDie(WIDTH-3,0,2, 0,0);
    //drawDie(WIDTH-3,HEIGHT/2-2,"6", 0);
    drawDie(WIDTH-3,HEIGHT-3,4, 0,0);
    inBorderedScreen=true;
  }
}

/// @brief Shows information about the game
void showHelpScreen() {
  static unsigned char y;
  
  resetScreenWithBorder();

  drawTextAlt(WIDTH/2-10,1,"how to play fujiTZEE");
  drawLine(WIDTH/2-10,2,20);
  y=3;

                   //1234567890123456789012345678901234567890
  // y++;drawText(5,y, "this is a dice game where players");
  // y++;drawText(5,y, "take turns rolling 5 dice up to 3");
  // y++;drawText(5,y, "times per turn to achieve specific");
  // y++;drawText(5,y, "combos, such as three of a kind,");
  // y++;drawText(5,y, "full house, or a straight. each");
  // y++;drawText(5,y, "combination corresponds to a score");
  // y++;drawText(5,y, "on the player's scorecard. Players");
  // y++;drawText(5,y, "must strategically choose which");
  // y++;drawText(5,y, "combo to score after each roll.");
  // y++;drawText(5,y, "the game continues until all 13");
  // y++;drawText(5,y, "score categories are filled, and");
  // y++;drawText(5,y, "the player with the highest total");
  // y++;drawText(5,y, "score at the end wins!");

  // 1400 bytes or so
                      //1234567890123456789012345678901234567890
  // y++;drawTextAlt(3,y, "TAKE TURNS ROLLING DICE UP TO three");
  // y++;drawTextAlt(3,y, "TIMES PER turn TO GET COMBINATIONS");
  // y++;drawTextAlt(3,y, "LIKE A set, full house, OR run.");
  // y++;
  // y++;drawTextAlt(3,y, "score YOUR COMBO TO EARN POINTS.");
  // y++;
  // y++;drawTextAlt(3,y, "PLAY CONTINUES FOR 13 ROUNDS UNTIL");
  // y++;drawTextAlt(3,y, "THE SCORESHEET IS FILLED.");
  
  // y++;
  // y++;drawTextAlt(3,y, "THE PLAYER WITH THE highest TOTAL");
  // y++;drawTextAlt(3,y, "SCORE WINS THE GAME!");

  // y++;y++;
  // centerTextAlt(y, "turn steps");

  // y+=2;drawTextAlt(7,y, "1. SELECT ANY DICE TO keep");
  // y++;drawTextAlt(7,y, "2. roll TO GET NEW DICE");
  // y++;drawTextAlt(7,y, "3. YOU MAY REPEAT STEPS 1-2");
  // y++;drawTextAlt(7,y, "4. CHOOSE YOUR score");

  // centerStatusText("PRESS ANY KEY FOR score INFO");

  // clearCommonInput();
  // cgetc();

  // resetScreenWithBorder();

  // drawTextAlt(11,1,"scoring your rolls");
  // drawLine(11,2,18);
  
  // y=4;centerTextAlt(y, "upper scores");
  // y+=2;drawText(5,y, "total all dice that match the");
  // y++;drawText(5,y, "number");

  // y+=2;centerTextAlt(y, "upper bonus");
  // y+=2;drawTextAlt(5,y, "IF UPPER TOTAL IS 63 OR HIGHER");
  // y++;drawTextAlt(5,y, "YOU SCORE A bonus 35 POINTS");

  // y+=3;centerTextAlt(y, "bottom scores");
  // y+=2;drawTextAlt(4,y, "set   - x OF A KIND - ADD ALL DICE");
  // y+=1;drawTextAlt(4,y, "house - FULL HOUSE - SET OF 2 AND 3");
  // y+=1;drawTextAlt(4,y, "run   - RUN OF x - EXAMPLE 12345");
  // y+=1;drawTextAlt(4,y, "count - SCORE TOTAL OF ALL DICE");
  // y+=1;drawTextAlt(4,y, "      - 5 OF A KIND");
  // drawFujzee(4,y);

  centerStatusText("press any key to close");

  //clearCommonInput();
  cgetc();
}

/// @brief Action called in Welcome Screen to check if a server name is stored in an app key
void welcomeActionVerifyServerDetails() {
  // Read server endpoint into server and query
  read_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, tempBuffer);
  
  i=strlen(tempBuffer);
  if (i) {
  
    // Split "server?query" into separate variables  
    while(--i) {
      if (tempBuffer[i]=='?') {
        strcpy(query, tempBuffer+i);
        tempBuffer[i]=0;
        strcpy(serverEndpoint, tempBuffer);
        break;
      }
    }
  }
}

void drawLogo(uint8_t x, uint8_t y)
{
  drawBox(x,y,8,1);
  drawTextAlt(x+1,y+1,"fujiTZEE");
}


void showPlayerGroupScreen() {
  static bool showPlayers;
  resetScreenWithBorder();
  drawBox(WIDTH/2-10,0,18,1);
  drawTextAlt(WIDTH/2-9,1,"LOCAL PLAYER SETUP");
  
  centerText(5,"up to 4 players can play on");
  centerText(7,"one system, sharing controls");

  centerTextAlt(10,"PRESS 1-4 TO EDIT PLAYER");

  centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE when done");
  
  clearCommonInput();
  showPlayers = true;

  while (!input.trigger ) {
    readCommonInput();
    
    // Exit early
    if (input.key == 'q' || input.key == KEY_ESCAPE)
      break;

    // Edit player by number
    if (input.key >= '1' && input.key <= '4') {
      i = input.key-'0';
      if (i<=prefs.localPlayerCount) {
        showPlayerNameScreen(i);
        restoreScreen();
        showPlayers=true;
      } else {
        soundRelease();
      } 
    }

    // Add new player
    if (input.key == 'a' || input.key == 'A') {
      if (prefs.localPlayerCount<4) {
        // Make sure new name is empty
        memset(&prefs.localPlayer[prefs.localPlayerCount],0,9);
        prefs.localPlayerCount++;
        showPlayerNameScreen(prefs.localPlayerCount);
        restoreScreen();
        showPlayers=true;
      } else {
        soundRelease();
      }
    }

    waitvsync();
    if (showPlayers) {
      showPlayers = false;

      drawBox(WIDTH/2-8,PLAYER_BOX_TOP,14,2+prefs.localPlayerCount + (prefs.localPlayerCount<4 ? 2:0));
      for(i=0;i<prefs.localPlayerCount;i++) {
        itoa(i+1,tempBuffer, 10);
        drawText(WIDTH/2-6,PLAYER_BOX_TOP+2+i, tempBuffer);
        drawTextAlt(WIDTH/2-5,PLAYER_BOX_TOP+2+i,":");
        drawTextAlt(WIDTH/2-4,PLAYER_BOX_TOP+2+i,prefs.localPlayer[i].name);
      }
      if (prefs.localPlayerCount<4) 
        drawTextAlt(WIDTH/2-6,PLAYER_BOX_TOP+3+i, "A:add player");
    }
  }
  clearCommonInput();
}

void showPlayerNameScreen(uint8_t p) {
  static char* playerName;
  static bool canDelete, canCancel, duplicateNames;

  for (i=PLAYER_BOX_TOP;i<PLAYER_BOX_TOP+10;i++)
    drawSpace(WIDTH/2-8,i,16);
  saveScreen();
  resetScreenWithBorder();

  canDelete = prefs.localPlayerCount>1;

  if (p>0) {
    strcpy(tempBuffer,"player ");
    itoa(p,tempBuffer+7, 10);
    centerTextAlt(5,tempBuffer);
    p--;
    canCancel=true;
  } else {
    // First load of game
    tempBuffer[0]=p=canCancel=0;
    centerTextAlt(4, "WELCOME TO fujiTZEE!");
  }

  centerText(7, "enter your name:");
  centerText(15,  "your name must be");
  centerText(16,  "at least 2 letters.");

  if (canDelete) {
    centerTextAlt(20,"to REMOVE player, press  ");
    drawText(WIDTH/2+12,20,"/"); 
  }
  
  drawBox(WIDTH/2-5,10,PLAYER_NAME_MAX+1,1);

  // Copy name to local buffer in case 
  playerName=prefs.localPlayer[p].name;
  strcpy(tempBuffer, playerName);
  
  resetInputField();
  clearCommonInput();
  while (true) {

    while (!inputFieldCycle(WIDTH/2-4, 11, PLAYER_NAME_MAX, tempBuffer)) {

      // Check if we are deleting this player (hitting / or cancelling a new player)
      if (canDelete && (
          input.key == REMOVE_PLAYER_KEY || 
          (input.key == KEY_ESCAPE && strlen(playerName)==0))
        ) {
        
        // Clear this player's name
        memset(playerName,0,9);

        // Shuffle downward player to take place if available
        for (i=p+1;i<prefs.localPlayerCount;i++) {
          strcpy(prefs.localPlayer[i-1].name, prefs.localPlayer[i].name);
          memset(prefs.localPlayer[i].name,0,9);
        }
        prefs.localPlayerCount--;
        break;
      } else if (input.key == KEY_ESCAPE) {
        // Cancel editing
        break;
      }
    }

    if (input.key != KEY_RETURN) {
      break;
    }

    // Remove trailing space
    while (tempBuffer[strlen(tempBuffer)-1]==' ') {
      tempBuffer[strlen(tempBuffer)-1]=0;
    }

    // If length < 2, keep editing
    if (strlen(tempBuffer)<2)
      continue;

    // Check if new name already exists
    duplicateNames=false;
    for(i=0;i<prefs.localPlayerCount;i++) {
      if (i != p && strcmp(tempBuffer, prefs.localPlayer[i].name)==0) {
        centerTextAlt(13,  "that name already exists!");
        soundRelease();
        duplicateNames=true;
        break;
      }
    }
    if (!duplicateNames) {
      break;
    }

  }

  // reset input field in case we broke early from the input loop (remove, cancel)
  resetInputField();

  if (input.key != KEY_ESCAPE) {
    
    // Copy from the temp buffer to the player name (if not deleting)
    if (input.key != REMOVE_PLAYER_KEY) {
      strcpy(playerName, tempBuffer);
    }

    // If player 1, write out to the main user app key
    if (p==0) {
      write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME,strlen(playerName), playerName);
    }

    // Save prefs, as other local player names are stored there
    savePrefs();
  }
}

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName() {
  static char *playerName;
  playerName = prefs.localPlayer[0].name;

  // Read player's name from app key
  read_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, tempBuffer);
  tempBuffer[8]=0;
  strcpy(playerName,tempBuffer);

  // Capture username if player didn't come in from the lobby
  if (!playerName[0]) {
    showPlayerNameScreen(0);
  } 

  // Convert to lowercase
  while (*playerName) {
    if (*playerName>=65 && *playerName<=90)
      *playerName+=32;
    playerName++;
  }
}

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen() {

  // Load preferences
  loadPrefs();

  // Retrieve the main player's name
  welcomeActionVerifyPlayerName();
  
  // Parse server url from app key if present 
  welcomeActionVerifyServerDetails();

  // If first run, show the help screen
  if (!prefs.seenHelp) {
    prefs.seenHelp=true;
    savePrefs();
    showHelpScreen();
  } 
  
  pause(30);
} 


/// @brief Shows a screen to select a table to join
void showTableSelectionScreen() {
  static uint8_t shownChip, tableIndex, altChip;
  static char* localQuery;
  static Table* table;
  state.inGame=tableIndex=altChip=0;
  
  resetScreenWithBorder();

  // An empty query means a table needs to be selected
  while (strlen(query)==0) {
    // Show names of local player(s)
  
    if (prefs.localPlayerCount==1) {
      strcpy(tempBuffer,"HELLO "); 
      strcat(tempBuffer, prefs.localPlayer[0].name);
      centerTextAlt(18, tempBuffer);
      centerTextAlt(20, "PRESS p TO ADD LOCAL PLAYERS");
      
      #ifdef COLOR_TOGGLE
      if (prefs.color)
        drawLine(12,21,1); // P
      #endif

    } else {
      centerTextAlt(18,"local players");
      strcpy(tempBuffer,"");
      for(i=0;i<prefs.localPlayerCount;i++) {
        strcat(tempBuffer, prefs.localPlayer[i].name);
        if (i<prefs.localPlayerCount-1) {
          strcat(tempBuffer, ", ");
        }
      }
      centerText(20, tempBuffer);
    }

    if (state.tableCount>0) {
      for(i=0;i<state.tableCount;++i) {
        drawSpace(6,9+i*2,WIDTH-8);
      }
    }
    waitvsync();
    centerText(12, "      refreshing game list..      ");
    drawLogo(WIDTH/2-5,0);
    
    centerText(4, "choose a game to join");
    drawText(6,7, "game");
    drawText(WIDTH-13,7, "players");
    drawLine(6,8,WIDTH-12);
    
    //waitvsync();

    state.tableCount = 0;
    if (apiCall("tables")) { 
      updateState(true);
    }

    if (state.tableCount>0) {
      drawSpace(6,12, WIDTH-12);
      for(i=0;i<state.tableCount;++i) {
        table = &state.tables[i];        
        j=9+i*2;
        k=WIDTH-6-strlen(table->players);
        
        drawTextAlt(k, j, table->players);
        drawTextAlt(6,j, table->name);
        
        if (table->players[0]>'0') {
          drawIcon(k-2, j, ICON_PLAYER);
        }
      }
    } else {
      centerText(12, "no servers are available");
    }

    centerStatusText("Refresh  Help  Players  Quit");
    
    #ifdef COLOR_TOGGLE
    if (prefs.color) {
      drawLine(6,HEIGHT,1); // R
      drawLine(15,HEIGHT,1); // H
      drawLine(21,HEIGHT,1); // P
      drawLine(30,HEIGHT,1); // Q
    }
    #endif
    
    shownChip=!state.tableCount;

    clearCommonInput();
    while (!input.trigger || !state.tableCount) {

      if (state.tableCount) {
        drawIcon(4,9+tableIndex*2, altChip<50 ? ICON_MARK : ICON_MARK_ALT);
      } 

      waitvsync();
      altChip=(altChip+1) % 60;
      readCommonInput();
      
      if (input.key == 'h' || input.key == 'H') {
        saveScreen();
        showHelpScreen();
        restoreScreen();
      } else if (input.key == 'r' || input.key =='R') {
        drawBlank(4,9+tableIndex*2);
        break;
      } else if (input.key == 'c' || input.key =='C') {
        prefs.color = cycleNextColor();
        savePrefs();
        break;
      } else if (input.key == 's' || input.key =='S') {
        prefs.disableSound = !prefs.disableSound;  
        soundCursor();
        savePrefs();
      } else if (input.key == 'p' || input.key =='P') {
        showPlayerGroupScreen();
        resetScreenWithBorder();
        break;
      } else if (input.key == 'q' || input.key =='Q') {
        quit();
      } /*else if (input.key != 0) {
        itoa(input.key, tempBuffer, 10);
        drawStatusText(tempBuffer);
      }*/
      
      if (!shownChip || (state.tableCount>0 && input.dirY)) {
        // Visually unselect old table
        table = &state.tables[tableIndex];
        j=9+tableIndex*2;
        drawBlank(4,j);
        drawTextAlt(6,j, table->name);
        drawTextAlt(WIDTH-6-strlen(table->players), j, table->players);

        // Move table index to new table
        tableIndex = (input.dirY+tableIndex+state.tableCount) % state.tableCount;

        // Visually select new table
        table = &state.tables[tableIndex];
        j=9+tableIndex*2;
        drawIcon(4,j, ICON_MARK);
        drawText(6,j, table->name);
        drawText(WIDTH-6-strlen(table->players), j, table->players);

        soundCursor();

        // Housekeeping - allows platform specific housekeeping, like stopping Attract/screensaver mode in Atari
        housekeeping();

        shownChip=1;
      }
    }
    
    if (input.trigger) {
      soundScore();

      // Clear screen and write server name
      resetScreenWithBorder();
      centerText(15, state.tables[tableIndex].name);
      
      strcpy(query, "?table=");
      strcat(query, state.tables[tableIndex].table);
      strcpy(tempBuffer, serverEndpoint);
      strcat(tempBuffer, query);

      //  Update server app key in case of reboot 
      write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, strlen(tempBuffer), tempBuffer);

    }
  }
  
  
  centerText(17, "connecting to server");
  progressAnim(19);
  
  // Reset the game state
  clearRenderState();
  state.waitingOnEndGameContinue = false;
  state.drawBoard = true;
  
  // The query will have the table already, e.g. "?table=1234"

  // Build the queries for each local player
  for(i=0;i<prefs.localPlayerCount;i++) {
    localQuery = state.localPlayer[i].query;
    // Append the table
    strcpy(localQuery, query);

    // When there is more than one local player, send a pov to lock in the visual order
    // so the board view is constant from player to player
    if (prefs.localPlayerCount>1) {
      strcat(localQuery,"&pov=");
      strcat(localQuery,prefs.localPlayer[0].name);
    }

    strcat(localQuery,"&player=");
    strcat(localQuery, prefs.localPlayer[i].name);
  
    // Replace space with + in query
    while (j=*(++localQuery)) {
      if (j==' ')
        *localQuery='+';
    }  
  }
 
  // Join all local players to table
  apiCallForAll("state");
  
  // Reduce wait count for an immediate call
  state.apiCallWait=0;

  state.inGame=true;
}

/// @brief shows in-game menu
void showInGameMenuScreen() {
  static uint8_t y;
  saveScreen();
  state.inGame =false;
  i=1;
  while (i) {
    
    resetScreenWithBorder();
    
    y = HEIGHT/2-5;
      
    drawBox(INGAME_MENU_X-3,y-2,21,11);
    drawTextAlt(INGAME_MENU_X,y,    "  Q: quit table");
    drawTextAlt(INGAME_MENU_X,y+=2, "  H: how to play"); 
    drawTextAlt(INGAME_MENU_X,y+=2, "  C: color mode");
    drawTextAlt(INGAME_MENU_X,y+=2, prefs.disableSound ?  "  S: sound OFF" : "  S: sound ON");
    drawTextAlt(INGAME_MENU_X,y+=2, "ESC: keep playing"); 
    
    strcpy(tempBuffer,  "CURRENTLY AT ");
    strcat(tempBuffer, state.serverName);

    centerTextAlt(y+6, tempBuffer);

    clearCommonInput();
    i=1;
    while (i==1) {
      readCommonInput();
      switch (input.key) {
        case 's':
        case 'S':
          prefs.disableSound = !prefs.disableSound;
          drawTextAlt(INGAME_MENU_X,y-2, prefs.disableSound ?  "  S: sound OFF" : "  S: sound ON ");
          soundScore();
          savePrefs();
          break;
        case 'c':
        case 'C':
            prefs.color = cycleNextColor();
            state.drawBoard = true;
            savePrefs();
            i=2;
            break;
        case 'h':
        case 'H':
          showHelpScreen();
        case KEY_ESCAPE:
        case KEY_ESCAPE_ALT:
          i=0;
          break;
        case 'q':
        case 'Q':
          resetScreenWithBorder();
          centerText(10, "please wait");

          // Inform server player is leaving
          apiCallForAll("leave");
          progressAnim(12);
          
          //  Clear server app key in case of reboot 
          write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER,0, "");

          // Clear query so a new table will be selected
          strcpy(query,"");
          showTableSelectionScreen();
          return;
      }
    }
  }

  // Show game screen again before returning
  
  clearCommonInput();
  
  state.inGame =true;
  if (state.drawBoard) {
    clearRenderState();
    setHighlight(-1,0,0);
    processStateChange();
  } else {
    restoreScreen();
  }
}


