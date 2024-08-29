#ifdef _CMOC_VERSION_
#include <cmoc.h>
#else
#include <stdlib.h>
#endif /* _CMOC_VERSION_ */
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

/// @brief Convenience function to reset screen and draw border
void resetScreenWithBorder() {
  resetScreen();
  
  // Draw dice corners
  drawDie(0,0,1, 0,0);
  //drawDie(0,HEIGHT/2-2,"5", 0);
  drawDie(0,HEIGHT-3,3, 0,0);

  drawDie(WIDTH-3,0,2, 0,0);
  //drawDie(WIDTH-3,HEIGHT/2-2,"6", 0);
  drawDie(WIDTH-3,HEIGHT-3,4, 0,0);
}

/// @brief Shows information about the game
void showHelpScreen() {
  static unsigned char y;
  
  resetScreenWithBorder();

  drawTextAlt(10,1,"how to play fujitzee");
  drawLine(10,2,20);
  y=3;
  
  y++;drawText(5,y, "players take turns rolling five");
  y++;drawText(5,y, "dice over 13 rounds to fill out");
  y++;drawText(5,y, "their score sheet.");

  y+=3;
  centerTextAlt(y, "winning");

  y+=2;drawText(5,y, "the player with the highest");
  y++;drawText(5,y, "score wins the game!");

  y+=3;
  centerTextAlt(y, "turn details");

  y+=2;drawTextAlt(5,y, "1. SELECT ANY DICE TO KEEP");
  y++;drawTextAlt(5,y, "2. roll TO GET NEW DICE");
  y++;drawTextAlt(5,y, "3. REPEAT UP TO TWO TIMES MORE");
  y++;drawTextAlt(5,y, "4. CHOOSE YOUR score");

  centerStatusText("PRESS ANY KEY FOR score INFO");

  clearCommonInput();
  cgetc();

  resetScreenWithBorder();

  drawTextAlt(11,1,"scoring your rolls");
  drawLine(11,2,18);
  
  y=4;centerTextAlt(y, "upper scores");
  y+=2;drawText(5,y, "total all dice that match the");
  y++;drawText(5,y, "number");

  y+=2;centerTextAlt(y, "upper bonus");
  y+=2;drawTextAlt(5,y, "IF UPPER TOTAL IS 63 OR HIGHER");
  y++;drawTextAlt(5,y, "YOU SCORE A bonus 35 POINTS");

  y+=3;centerTextAlt(y, "bottom scores");
  y+=2;drawTextAlt(4,y, "set   - x OF A KIND - ADD ALL DICE");
  y+=1;drawTextAlt(4,y, "house - FULL HOUSE - SET OF 2 AND 3");
  y+=1;drawTextAlt(4,y, "run   - RUN OF x - EXAMPLE 12345");
  y+=1;drawTextAlt(4,y, "count - SCORE TOTAL OF ALL DICE");
  y+=1;drawTextAlt(4,y, "      - 5 OF A KIND");
  drawFujzee(4,y);

  y+=3;
    y+=3;
  
  centerStatusText("press any key to close");

  clearCommonInput();
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
  drawBox(5,0,28,1);
  drawTextAlt(6,1,"fujiTZEE: LOCAL PLAYER SETUP");
  
  centerText(5,"up to 4 players can play from the");
  centerText(7,"the same system, sharing controls");

  centerTextAlt(10,"press 1-4 to edit/remove player");

  centerTextAlt(HEIGHT-2,"PRESS trigger/space WHEN DONE");
  
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
        saveScreen();
        showPlayerNameScreen(i);
        restoreScreen();
        showPlayers=true;
      } else {
        soundRelease();
      } 
    }

    // Add new player
    if (input.key == 'a') {
      if (prefs.localPlayerCount<4) {
        // Clear name
        strcpy(&prefs.localPlayer[prefs.localPlayerCount],"");
        prefs.localPlayerCount++;
        saveScreen();
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

      for (i=PLAYER_BOX_TOP;i<PLAYER_BOX_TOP+10;i++)
        drawSpace(WIDTH/2-8,i,16);

      drawBox(WIDTH/2-8,PLAYER_BOX_TOP,14,2+prefs.localPlayerCount + (prefs.localPlayerCount<4 ? 2:0));
      for(i=0;i<prefs.localPlayerCount;i++) {
        itoa(i+1,tempBuffer, 10);
        drawTextAlt(WIDTH/2-6,PLAYER_BOX_TOP+2+i, tempBuffer);
        drawText(WIDTH/2-5,PLAYER_BOX_TOP+2+i,":");
        drawText(WIDTH/2-4,PLAYER_BOX_TOP+2+i,prefs.localPlayer[i].name);
      }
      if (prefs.localPlayerCount<4) 
        drawTextAlt(WIDTH/2-6,PLAYER_BOX_TOP+3+i, "a:ADD PLAYER");
    }
  }
  clearCommonInput();
}

void showPlayerNameScreen(uint8_t p) {
  static char* playerName;
  static bool canDelete;
  resetScreenWithBorder();

  canDelete = prefs.localPlayerCount>1;

  if (p>0) {
    strcpy(tempBuffer,"player ");
    itoa(p,tempBuffer+7, 10);
    centerTextAlt(5,tempBuffer);
    p--;
  } else {
    // First load of game
    tempBuffer[0]=p=0;
    centerTextAlt(4, "WELCOME TO fujiTZEE!");
  }

  centerText(7, "enter your name:");
  centerText(17,  "name must be at least 2 letters.");

  if (canDelete) {
    centerTextAlt(20,"to REMOVE this player, press ESC");
  }

  drawBox(15,10,PLAYER_NAME_MAX+1,1);

  playerName = prefs.localPlayer[p].name;
  
  clearCommonInput();
  while (!inputFieldCycle(16, 11, PLAYER_NAME_MAX, playerName, canDelete));
  
  if (input.key == KEY_ESCAPE && canDelete) {
    // Clear this player's name
    memset(playerName,0,9);

    // Shuffle downward player to take place if available
    for (i=p+1;i<prefs.localPlayerCount;i++) {
      strcpy(&prefs.localPlayer[i-1].name, &prefs.localPlayer[i].name);
      memset(&prefs.localPlayer[i].name,0,9);
    }
    prefs.localPlayerCount--;
  }
  
  if (p==0) {
    write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME,strlen(playerName), playerName);
  }
  savePrefs();
}

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName() {
  static char* playerName;
  playerName = prefs.localPlayer[0].name;

  // Read player's name from app key
  read_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, tempBuffer);  
  tempBuffer[8]=0;
  strcpy(playerName,tempBuffer);


  // Convert to lowercase
  for(i=0;i<strlen(playerName);i++) {
    if (playerName[i] >=65 && playerName[i]<=90)
      playerName[i]+=32;
  }
  
  // Capture username if player didn't come in from the lobby
  if (strlen(playerName) == 0)
    showPlayerNameScreen(0);
}

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen() {
  //resetScreenWithBorder();
  //drawLogo(WIDTH/2-5,3);
  
  loadPrefs();

  //centerText(13,"reading sdcard appkeys");
  //centerText(13,"this may take up to a minute");

  welcomeActionVerifyPlayerName();
  welcomeActionVerifyServerDetails();

  //strcpy(tempBuffer, "welcome ");
  //strcat(tempBuffer, prefs.localPlayer[0].name);
  //centerText(14,tempBuffer);  
  
  //pause(45);

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
  static uint8_t shownChip, tableIndex, waitTime, altChip;
  tableIndex=waitTime=altChip=0;
  
  // An empty query means a table needs to be selected
  while (strlen(query)==0) {

    // Show the status immediately before retrival
    resetScreenWithBorder();
    drawLogo(WIDTH/2-5,0);
    centerStatusText("refreshing game list..");
    centerText(4, "choose a game to join");
    drawText(6,7, "game");
    drawText(WIDTH-13,7, "players");
    drawLine(6,8,WIDTH-12);
   
    pause(waitTime);
    waitvsync();
   
    if (apiCall("tables")) { 

      // Add an artifical wait time if refreshing
      waitTime=20;
      
      updateState(true);
      if (state.tableCount>0) {
        for(i=0;i<state.tableCount;++i) {
          drawTextAlt(6,9+i*2, state.tables[i].name);
          drawTextAlt(WIDTH-6-strlen(state.tables[i].players), 9+i*2, state.tables[i].players);
          if (state.tables[i].players[0]>'0') {
            drawText(WIDTH-6-strlen(state.tables[i].players)-2, 9+i*2, "*");
          }
        }
      } else {
        centerTextAlt(12, "sorry, no servers are available");
      }

      //drawStatusText("r>efresh   h+elp  c:olor   n+ame   q+uit");
      //centerStatusText("Refresh Help Color Name Quit");
      //centerStatusText("Refresh  Help  Name  Quit");
      centerStatusText("Refresh  Help  Players  Quit");
      
      shownChip=0;

      clearCommonInput();
      while (!input.trigger || !state.tableCount) {
        if (altChip<50)
          drawMark(4,9+tableIndex*2);
        else
          drawAltMark(4,9+tableIndex*2);

        waitvsync();
        altChip=(altChip+1) % 60;
        readCommonInput();
       
        if (input.key == 'h' || input.key == 'H') {
          saveScreen();
          showHelpScreen();
          restoreScreen();
        } else if (input.key == 'r' || input.key =='R') {
          break;
        } else if (input.key == 'c' || input.key =='C') {
          prefs.color = cycleNextColor();
          savePrefs();
          break;
         } else if (input.key == 'p' || input.key =='P') {
          showPlayerGroupScreen();
          break;
        } else if (input.key == 'q' || input.key =='Q') {
          quit();
        } /*else if (input.key != 0) {
          itoa(input.key, tempBuffer, 10);
          drawStatusText(tempBuffer);
        }*/
        
        if (!shownChip || (state.tableCount>0 && input.dirY)) {

          drawBlank(4,9+tableIndex*2);
          drawTextAlt(6,9+tableIndex*2, state.tables[tableIndex].name);
          drawTextAlt(WIDTH-6-strlen(state.tables[tableIndex].players), 9+tableIndex*2, state.tables[tableIndex].players);

          tableIndex+=input.dirY;
          if (tableIndex==255) 
            tableIndex=state.tableCount-1;
          else if (tableIndex>=state.tableCount)
            tableIndex=0;

          drawMark(4,9+tableIndex*2);
          drawText(6,9+tableIndex*2, state.tables[tableIndex].name);
          drawText(WIDTH-6-strlen(state.tables[tableIndex].players), 9+tableIndex*2, state.tables[tableIndex].players);

          soundCursor();

          // Housekeeping - allows platform specific housekeeping, like stopping Attract/screensaver mode in Atari
          housekeeping();

          shownChip=1;
        }
      }
      
      if (input.trigger) {
        soundCursor();

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
  }
  
  centerText(17, "connecting to server");
  progressAnim(19);
  
  // Reset the game state
  clearRenderState();
  state.waitingOnEndGameContinue = false;
  state.drawBoard = true;
  
  // The query will have the table already, e.g. "?table=1234"
  strcat(query, "&player=");
  strcat(query, prefs.localPlayer[0].name);
  
  // Replace space with + for player name
  i=strlen(query);
  while(--i)
    if (query[i]==' ')
      query[i]='+';
  
  // Reduce wait count for an immediate call
  state.apiCallWait=0;
}

/// @brief shows in-game menu
void showInGameMenuScreen() {
  saveScreen();
  i=1;
  while (i) {
    
    resetScreenWithBorder();
    
    x = WIDTH/2-8;
    y = HEIGHT/2-5;
      
    drawBox(x-3,y-2,21,9);
    drawTextAlt(x,y,    "  Q: quit table");
    drawTextAlt(x,y+=2, "  H: how to play"); 
    drawTextAlt(x,y+=2, "  C: color toggle");
    drawTextAlt(x,y+=2, "ESC: keep playing"); 
    
    strcpy(tempBuffer,  "CURRENTLY AT ");
    strcat(tempBuffer, state.serverName);

    centerTextAlt(y+6, tempBuffer);

    clearCommonInput();
    i=1;
    while (i==1) {
      readCommonInput();
      switch (input.key) {
        case 'c':
        case 'C':
            prefs.color = cycleNextColor();
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
          apiCall("leave");
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
  restoreScreen();
}


