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

/// @brief Convenience function to reset screen and draw border
void resetScreenWithBorder() {
  resetScreen();
  
  // Draw dice corners
  drawDie(0,0,1, 0);
  //drawDie(0,HEIGHT/2-2,"5", 0);
  drawDie(0,HEIGHT-3,3, 0);

  drawDie(WIDTH-3,0,2, 0);
  //drawDie(WIDTH-3,HEIGHT/2-2,"6", 0);
  drawDie(WIDTH-3,HEIGHT-3,4, 0);
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

  resetScreen();
  
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

void drawLogo()
{
  centerText(4, "fujitzee!");
}

void showPlayerNameScreen() {
  
  resetScreenWithBorder();
  drawLogo();

  drawText(13,13, "enter your name:");
  drawBox(15,16,PLAYER_NAME_MAX+1,1);
  //drawText(16,17, playerName);
  //i=strlen(playerName);
  
  clearCommonInput();
  //while (input.key != KEY_RETURN || i<2) {
  while (!inputFieldCycle(16, 17, PLAYER_NAME_MAX, playerName)) ;
  
  
  for (y=13;y<19;++y)
    drawText(13,y, "                 ");
  
  

  write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, playerName);
}

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName() {
  // Read player's name from app key
  read_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, tempBuffer);  
  tempBuffer[12]=0;
  strcpy(playerName,tempBuffer);


  // Convert to lowercase
  for(i=0;i<strlen(playerName);i++) {
    if (playerName[i] >=65 && playerName[i]<=90)
      playerName[i]+=32;
  }
  


  // Capture username if player didn't come in from the lobby
  if (strlen(playerName) == 0)
    showPlayerNameScreen();
}

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen() {
  resetScreenWithBorder();
  drawLogo();
  
  loadPrefs();
  welcomeActionVerifyPlayerName();
  welcomeActionVerifyServerDetails();

  strcpy(tempBuffer, "welcome ");
  strcat(tempBuffer, playerName);
  centerText(13,tempBuffer);  
  
  pause(45);

  // If first run, show the help screen
  if (prefs[PREF_HELP]!=2) {
    prefs[PREF_HELP]=2;
    savePrefs();
    showHelpScreen();
  } 
  pause(30);
} 

/// @brief Action in Table Selection Screen, joins the selected server
void tableActionJoinServer() {
  // Reset the game state
  clearRenderState();
  
  strcat(query, "&player=");
  strcat(query, playerName);
  
  // Replace space with + for player name
  i=strlen(query);
  while(--i)
    if (query[i]==' ')
      query[i]='+';
  
}

/// @brief Shows a screen to select a table to join
void showTableSelectionScreen() {
  static uint8_t shownChip, tableIndex, waitTime;
  tableIndex=waitTime=0;
  
  // An empty query means a table needs to be selected
  while (strlen(query)==0) {

    // Show the status immediately before retrival
    resetScreenWithBorder();
    centerStatusText("refreshing game list..");
    centerText(3, "choose a game to join");
    drawText(6,6, "game");
    drawText(WIDTH-13,6, "players");
    drawLine(6,7,WIDTH-12);
    
    waitvsync();
    pause(waitTime);
  
    if (apiCall("tables", false)) {

      // Add an artifical wait time if refreshing
      waitTime=20;
      
      updateState(true);
      if (state.tableCount>0) {
        for(i=0;i<state.tableCount;++i) {
          drawTextAlt(6,8+i*2, state.tables[i].name);
          drawTextAlt(WIDTH-6-strlen(state.tables[i].players), 8+i*2, state.tables[i].players);
          if (state.tables[i].players[0]>'0') {
            drawText(WIDTH-6-strlen(state.tables[i].players)-2, 8+i*2, "*");
          }
        }
      } else {
        centerTextAlt(12, "sorry, no servers are available");
      }

      //drawStatusText("r>efresh   h+elp  c:olor   n+ame   q+uit");
      //centerStatusText("Refresh Help Color Name Quit");
      centerStatusText("Refresh  Help  Name  Quit");
      
      shownChip=0;

      clearCommonInput();
      while (!input.trigger || !state.tableCount) {
        waitvsync();
        readCommonInput();
       
        if (input.key == 'h' || input.key == 'H') {
          showHelpScreen();
          break;
        } else if (input.key == 'r' || input.key =='R') {
          break;
        } else if (input.key == 'c' || input.key =='C') {
          prefs[PREF_COLOR] = cycleNextColor()+1;
          savePrefs();
          break;
         } else if (input.key == 'n' || input.key =='N') {
          showPlayerNameScreen();
          break;
        } else if (input.key == 'q' || input.key =='Q') {
          quit();
        } /*else if (input.key != 0) {
          itoa(input.key, tempBuffer, 10);
          drawStatusText(tempBuffer);
        }*/
        
        if (!shownChip || (state.tableCount>0 && input.dirY)) {

          drawBlank(4,8+tableIndex*2);
          tableIndex+=input.dirY;
          if (tableIndex==255) 
            tableIndex=state.tableCount-1;
          else if (tableIndex>=state.tableCount)
            tableIndex=0;

          drawChip(4,8+tableIndex*2);

          soundCursor();
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
        write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, tempBuffer);

      }
    }
  }
  
  centerText(17, "connecting to server");
  
  
  progressAnim(19);
  
  tableActionJoinServer();
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
            prefs[PREF_COLOR] = cycleNextColor()+1;
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
          apiCall("leave", false);
          progressAnim(12);
          
          //  Clear server app key in case of reboot 
          write_appkey(AK_LOBBY_CREATOR_ID,  AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, "");

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


