#include <stdlib.h>
#include <string.h>
#include "platform-specific/graphics.h"
#include "platform-specific/sound.h"
#include "platform-specific/util.h"
#include "gamelogic.h"
#include "misc.h"
#include "stateclient.h"
#include "screens.h"
#include <stdio.h>
#include <peekpoke.h>

uint8_t chat[20]="";
//uint8_t scoreY[] = {1,2,3,4,5,6, 8, 9,11,12,13,14,15,16,17,19};
uint8_t scoreY[] =   {3,4,5,6,7,8,10,11,13,14,15,16,17,18,19,21};
char* scores[]={"one","two","three","four","five","six","total","bonus","set 3","set 4","house","s run","l run","count"};
uint8_t cursorPos, prevCursorPos, spectators, inputField_done, validX;

void progressAnim(unsigned char y) {
  for(i=0;i<3;++i) {
    pause(10);
    drawMark(WIDTH/2-2+i*2,y);
  }
}

void processStateChange() {

  renderBoardNamesMessages();
  handleAnimation();

  state.prevPlayerCount=state.playerCount;
  state.prevActivePlayer = state.activePlayer;
  state.prevRollsLeft = state.rollsLeft;
  state.prevRound = state.round;
}


void renderBoardNamesMessages() {
  static bool redraw;
  static uint8_t scoreCursorX, scoreCursorY, c, len,maxScoreY, newScoreFound, ignoreNewScore, mostRecentPlayer, i4;
  static Player *player; 
  static int16_t score;
  
  if (state.drawBoard) {
    state.drawBoard = false;
    resetScreen();
    drawLogo(0,0);
    drawBoard();
  }


  // If player is waiting on end game screen, auto-ready up if the game is starting
  
  // Redraw the entire board on a new game
  redraw = state.round < state.prevRound;

  if (state.waitingOnEndGameContinue) {
    if (state.round == 1) {
      state.waitingOnEndGameContinue =false;
      // Force a redraw since the player waited on the end game screen until start
      redraw = true;
      clearRenderState();
    } else {
      
      if (state.countdownStarted)
        centerTextWide(HEIGHT-3,state.prompt);

      if (!state.countdownStarted && state.prompt[0]== 's') {
          soundJoinGame();
          if (state.players[0].scores[0] < 1) {
            apiCallForAll("ready");
          }
          
          state.countdownStarted = true;
        } else if (state.countdownStarted && state.prompt[0]!= 's') {
          state.countdownStarted = false;
        }
      return;
    }
  }
  
  // Clear score board if a new round
  if (redraw && state.round==0) {    
      // Clear score locations
    for (i=17;i<41;i+=4) {
      for(j=0;j<15;j++)  {
        drawSpace(i, scoreY[j], 3);
      }
    }
    //drawSpace(0,scoreY[15],40);
    clearBelowBoard();
    centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE to toggle");
  }


  // Draw player names if the count changed
  spectators=0;

  for(i=1;i<=PLAYER_MAX;i++) {
    y=i+2;
    x=14+i*4;
    
    if (i<=state.playerCount) {
      
      player = &state.players[i-1]; 
      c= player->name[player->alias];
      len = (uint8_t)strlen(player->name);
      
      if (player->scores[0]==-2) {
        // Draw spec icon in front of name 
        drawSpec(0,y);
        spectators++;
        // Clear initial/scoreboard for this player index if they were not previously viewing
        if (!player->isViewing && i<7) {
          drawBlank(x,1);
          for(j=0;j<15;j++)  {
            drawSpace(x-1, scoreY[j], 3);
          }
          player->isViewing=true;
        }
      } else { 
        player->isViewing=false;
        if (state.round>0 && state.activePlayer != i-1)
          drawBlank(0,y);
          // Player initials across top of screen
        drawCharAlt(x,1,c);
      }
      for (j=0;j<player->alias;j++) {
        drawChar(1+j,y,player->name[j]);
      }
      drawCharAlt(1+player->alias,y,c);
      drawText(2+player->alias,y,player->name+player->alias+1);
      drawSpace(1+len, y, 8-len);

    } else if (i<=state.prevPlayerCount) {
      // Blank out entries for this player
      drawSpace(0,y,9);
      
      // Blank scoreboard
      if (i<7) {
        drawBlank(x,1);
        for(j=0;j<15;j++)  {
          drawSpace(x-1, scoreY[j], 3);
        }
      }
    }

  }



  // Round 0 (waiting to start) checks, or going into round 1
  if (state.round ==0 || (state.round == 1 && state.prevRound==0)) {
    // Display "waiting for players" prompt if changed in ready mode
    if (state.round==0 && state.promptChanged) {
        centerTextWide(HEIGHT-3,state.prompt);
        state.promptChanged = false;
      if (!state.countdownStarted && state.prompt[0]== 's') {
        soundJoinGame();
        state.countdownStarted = true;
      } else if (state.prompt[0]!= 's') {
        state.countdownStarted = false;
      }
    }

    // Show players that are ready to start
    if (forceReadyUpdates) {
      for(i=0;i<6;i++) {
        i4=18+i*4;
        if (i<state.playerCount && state.players[i].scores[0]==1) {
          drawTextVert(i4,3,"ready");
          drawMark(0,i+3);
        } else {
          drawTextVert(i4,3,"     ");
          // Only clear the dice if it is still round LOBBY
          if (state.round == 0) {
            drawBlank(0,i+3);
          }
        }
      }

      forceReadyUpdates = 0;
    }
  }

  // Exit early as below text is for rounds > 0 
  if (state.round == 0)
    return;

  // Scoreboard and prompt - refresh if the active player changed, or round changed
  if (state.activePlayer != state.prevActivePlayer || state.round != state.prevRound || state.playerCount != state.prevPlayerCount) {
    
    // Remove score cursorPos
    if (cursorPos>10 && cursorPos<99) {
      drawBlank(validX,scoreY[cursorPos-10]);
      cursorPos = 99;
    }

    if (state.round == 99) {
      clearBelowBoard();

      // Only specify scores header if there are scores (game not aborted early)
      if (state.players[0].scores[15]>0) {
        drawText(11,21,"score");
      }
      setHighlight(-1,0,0);
    }


    // Update scores on-screen - two pass - first highlights in green the new one
    newScoreFound = 0;
    scoreCursorY=0;
   
    // Skip ahead to drawing the second round if the player hasn't changed
    ignoreNewScore= state.activePlayer == state.prevActivePlayer || redraw;
    
    // In case this client is lagging behind (multiple players scored since)
    // only animate the most recent player's score
    mostRecentPlayer = (state.activePlayer+state.playerCount-1) % state.playerCount;
    while (state.players[mostRecentPlayer].scores[0]==-2) {
      mostRecentPlayer = (mostRecentPlayer+state.playerCount-1) % state.playerCount;
    }

    for (i=state.playerCount-1;i<255;i--) {
      // Skip spectators or going beyond 6 players
      if (i>5 || state.players[i].scores[0]==-2)
        continue;
      h=17+i*4;
      i4=h+3;
      maxScoreY= state.round<99 ? 15 : 16;
      for (j=0;j<maxScoreY;j++) {
        score = state.players[i].scores[j];
        if (score>-1) {
          itoa(score, tempBuffer, 10);
          len=(uint8_t)strlen(tempBuffer);
          if (len<3) {
            drawSpace(h,scoreY[j],len);
          }
          if (j==15 || (!newScoreFound && !ignoreNewScore && i!= state.localPlayer[state.currentLocalPlayer].index && i==mostRecentPlayer && isEmpty(h+2, scoreY[j]))) {
            drawTextAlt(i4-len,scoreY[j],tempBuffer);
            if (scoreCursorY==0 ) {
              scoreCursorY=scoreY[j];
              scoreCursorX=h;
            }
            newScoreFound=1;
          } else {
            drawText(i4-len,scoreY[j],tempBuffer);
          }
        } else if (ignoreNewScore) {
          // Draw blank (just in case there was something there from a previous player)
          drawSpace(h,scoreY[j],3);
        }
      }
       
      //}
    }

    // Animate arrow showing newly added score
    if (!redraw && state.round<99 && scoreCursorY>0 && newScoreFound) {
      drawCursor(scoreCursorX,scoreCursorY, 0);
      pause(25);
      drawCursor(scoreCursorX,scoreCursorY, 1);
      soundScore();
      pause(5);
      drawCursor(scoreCursorX,scoreCursorY, 0);
      pause(30);
      drawBlank(scoreCursorX,scoreCursorY);
    }

     // Clear old turn indicator
    if (state.prevActivePlayer>-1)
      drawBlank(0,state.prevActivePlayer+3);
    
    // Draw new active player indicator
    if (state.activePlayer>-1) {
      drawMark(0,state.activePlayer+3);
      //if (state.activePlayer != 0)
      setHighlight(state.activePlayer, state.localPlayerIsActive, 0);
    } else {
      setHighlight(-1,0,0);
    }

    // Handle end of game
    if (state.round == 99) { 

      // Clear active indicators
      drawTextVert(0,3,"            ");
      centerText(HEIGHT-3, state.prompt);

      if (state.round != state.prevRound) {
        soundGameDone();

        pause(120);
        centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE to continue");
        state.waitingOnEndGameContinue = true;
        state.countdownStarted = false;
      } else {
        centerTextAlt(HEIGHT-1,"please wait..");
      }
        clearCommonInput();     
    } else {
      if (!state.localPlayerIsActive)
        pause(30);
      
      clearBelowBoard();
      if (spectators>0) {
        itoa(spectators, tempBuffer, 10);
        strcat(tempBuffer," watching");
        drawSpec(0,HEIGHT-1);
        drawTextAlt(2,HEIGHT-1, tempBuffer);
      } 

      // Visual hack - when playing multiple local players, put player's name + your turn
      if (state.localPlayerIsActive && prefs.localPlayerCount>1) {
        drawText(0,HEIGHT-4,"your turn");
        drawText(0,HEIGHT-3,state.players[state.activePlayer].name);
      } else {
        drawText(0,HEIGHT-4, state.prompt); 
      }
      
      pause(30);
    }

    if (state.localPlayerIsActive && !state.viewing) {
      setHighlight(state.activePlayer, true, 2);
      pause(5);
      soundMyTurn();
      soundMyTurn();
      pause(5);
      setHighlight(state.activePlayer, true, 0);
    }
  }
}

void handleAnimation() {
  static bool isThisPlayer;
  static uint8_t highScoreIndex, isFujitzee, i4;
  static int16_t score;

  waitvsync();  
  
  // Don't touch anything until player continues
  if (state.waitingOnEndGameContinue)
    return;

  isThisPlayer = !state.viewing && state.localPlayerIsActive;

  // Setup the player input details if this is a new roll
  if (state.rollsLeft != state.prevRollsLeft || state.activePlayer != state.prevActivePlayer )  {
    state.rollFrames=31;
    if (isThisPlayer) {
      state.playerMadeMove=false;
      prevCursorPos=5;
      cursorPos=1;
      validX=17+state.activePlayer*4;


      // If no rolls are remaining, default the cursorPos on the highest score
      if (state.rollsLeft==0) {
        highScoreIndex = 0;
        for (j=1;j<15;j++) {
          if (state.validScores[j]>state.validScores[highScoreIndex])
            highScoreIndex = j;
        }
        cursorPos=10+highScoreIndex;
      }
    }
  }

  if (state.activePlayer<0 || !state.rollFrames)
    return;

  state.rollFrames--;
  
  // Check for Fujitzee roll (all 5 dice match each other)
  // To display special animation
  if (state.rollFrames==0) {
    for (i=1;i<5;i++) {
      if (state.dice[i]!=state.dice[0])
        break;
    }
    isFujitzee=i==5;
  } else {
    isFujitzee=0;
  }
  
  // Draw the dice, randomly displaying the ones that are currently being rolled
  if (state.rollFrames % 4==0)
    soundRollDice();


  for(j=0;j<=isFujitzee;j++) {

    // Play fujitzee sound when rolled, then clear flag so dice return to white
    if (isFujitzee && j) {
      soundFujitzee();
      isFujitzee=0;
    }
    for(i=0;i<5;i++) {
      i4=20+4*i;
      if (state.rollFrames && state.keepRoll[i]=='1' ) {
        // Draw a random die
        drawDie(i4,HEIGHT-4,rand()%6+1,0,isFujitzee);
      } else {
        // Draw the kept die
        drawDie(i4,HEIGHT-4,state.dice[i]-48,(state.rollFrames || state.rollsLeft>0) && state.keepRoll[i]=='0', isFujitzee);
      }
    }

    soundStop();
  }


  // If the rolling has stopped and this player is playing
  if (isThisPlayer && !state.rollFrames) {

    // Display the potential scores for this player
    for (j=0;j<15;j++) {
      score= state.validScores[j];
      if (score>=0) {
        if (score>0) {
          itoa(score, tempBuffer, 10);
          drawSpace(validX,scoreY[j],3-strlen(tempBuffer)); 
        } else {
          strcpy(tempBuffer,"  ");
        }

        drawTextAlt(validX+3-strlen(tempBuffer),scoreY[j],tempBuffer);
      } 
    }

    // Draw "Rolls" die if this player's turn and there are rolls left
    drawDie(15,HEIGHT-4,state.rollsLeft+13,0,0);
  }
}

void processInput() {
  readCommonInput();

  if (state.waitingOnEndGameContinue) {
    if (input.trigger) {
      state.waitingOnEndGameContinue = false;
      clearBelowBoard();
      clearRenderState();
    }
  } else if (!state.viewing) {
    // Toggle readiness if waiting to start game
    if (state.round == 0 && input.trigger) {
      j=!state.players[0].scores[0];
      for(i=0;i<prefs.localPlayerCount;i++) {
        state.players[state.localPlayer[i].index].scores[0] = j;
      }
      forceReadyUpdates=true;
      renderBoardNamesMessages();
      if (state.players[0].scores[0]) 
        soundScore();
      else
        soundScoreCursor();

      apiCallForAll("ready");
      return;
    }

    // Wait on this player to make roll decisions
    if (!state.rollFrames && state.localPlayerIsActive && !state.playerMadeMove && state.round>0) {
      waitOnPlayerMove();
    }
  }

  switch(input.key) {
      case KEY_ESCAPE: // Esc
      case KEY_ESCAPE_ALT: // Esc Alt
        showInGameMenuScreen();  
        break;
    }    
}

void waitOnPlayerMove() {
  static int jifsPerSecond;
  static bool foundValidLocation;
  static uint8_t waitCount, frames;
  
  resetTimer();

  // Determine max jiffies for PAL and NTS
  jifsPerSecond=PEEK(0xD014)==1 ? 50 : 60;

  maxJifs = jifsPerSecond*state.moveTime;
  waitCount=frames=0;
  
  // Move selection loop
  while (state.moveTime>0) {
      frames=(frames+1) % 30;
      waitvsync();
        
      // Update horizontal cursorPos location - dice: 0 = roll, 1-5 equal select dice at that index
      if (input.dirX !=0 && state.rollsLeft) {
        // If cursorPos was selecting a score, bring it back down
        if (cursorPos>9)
          cursorPos = 1;

        cursorPos+=input.dirX;

        // Bounds check
        if (cursorPos>5)
          cursorPos = prevCursorPos;
      } else if (input.dirY == 1 && cursorPos<10) {
        // If pressing down on dice cursor, navigate directly to roll button.
        cursorPos=0;
        
        // Update vertical cursorPos location - scores: 10-24 represent the possible scoring locations
      }else if (input.dirY != 0) {
        // If cursorPos was selecting dice, bring it up
        if (cursorPos<10)
          cursorPos=25;
        
        while (1) {
          cursorPos+=input.dirY;
          
          // Bounds checks
          foundValidLocation=true;
          if (cursorPos<10 || (cursorPos>24 && state.rollsLeft==0))
            cursorPos = prevCursorPos;
          else if (cursorPos>24)
            cursorPos=prevCursorPos <10 ? prevCursorPos : 1;
          else {
            // Skip over invalid scores
            if (state.validScores[cursorPos-10]<0)
              foundValidLocation=false;
          }

          // Break once we are at a valid location
          if (foundValidLocation)
            break;
        }
      }
     
    waitvsync();

    // Draw cursorPos
    if (cursorPos != prevCursorPos) {
      
      // Hide cursorPos 
      if (prevCursorPos < 6)
        hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+16);
      else {
        h=prevCursorPos-10;
        drawBlank(validX,scoreY[h]);
        if (state.validScores[h]==0) {
          drawBlank(validX+2,scoreY[h]);
        }
      }

      // Draw cursorPos
      if (cursorPos < 6) {
        drawDiceCursor(4*cursorPos-(cursorPos==0)+16);
        soundCursor();
      } else {
        h=cursorPos-10;
        drawCursor(validX,scoreY[h],0);
        if (state.validScores[h]==0) {
          drawTextAlt(validX+2,scoreY[h],"0");
        }
        soundScoreCursor();
      }
      prevCursorPos = cursorPos;
      
      
    } else if (cursorPos>9){ 
      drawCursor(validX,scoreY[cursorPos-10],(frames<3)*0x80);
    }

    // Handle trigger press
    if (input.trigger) {
      
      if (cursorPos>=10) {
        // Select score
        i=cursorPos-10;

        // Cursor select animation 
        drawCursor(validX,scoreY[cursorPos-10],1);soundScore();
        
        // First, hide all scores but the main score
        for (j=0;j<15;j++) {
          if (state.validScores[j]>0) {
            if (j != i)
              drawSpace(validX,scoreY[j],3);
          }
        }

        pause(5);drawCursor(validX,scoreY[cursorPos-10],0);; pause(10);

        // Send command to score this value
        strcpy(tempBuffer, "score/");
        itoa(i, tempBuffer+6, 10);
        sendMove(tempBuffer);

        state.playerMadeMove = true;
        //soundScore();
        return;
      } else if (cursorPos>0) {
        // Toggle kept state of die
        i = state.keepRoll[cursorPos-1]= state.keepRoll[cursorPos-1]=='1' ? '0' : '1';
        drawDie(16+cursorPos*4,HEIGHT-4,state.dice[cursorPos-1]-48,i == '0', 0);
        if (i=='0')
          soundKeep();
        else 
          soundRelease();
      } else {
        // Request another roll

        // Highlight the roll die in green
        drawDie(15,HEIGHT-4,state.rollsLeft+15,0,0);
        soundRollButton();
        
        strcpy(tempBuffer, "roll/");
        strcat(tempBuffer, state.keepRoll);
        sendMove(tempBuffer);

        state.playerMadeMove = true;
        hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+16);
        // Clear clock
        drawSpace(10,HEIGHT-4,3);
        return;
      }
    }
    
    // Tick counter once per second   
    if (++waitCount>5) {
      waitCount=0;
      i = (maxJifs-getTime())/jifsPerSecond;
      if (i<=15 && i != state.moveTime) {
        state.moveTime = i;
        tempBuffer[0]=' ';
        itoa(i, tempBuffer+1, 10);
        drawTextAlt(12-strlen(tempBuffer), HEIGHT-4, tempBuffer);
        drawClock(12,HEIGHT-4);
        soundTick();
      }
    } 

    // Pressed Esc
    switch (input.key) { 
      case KEY_ESCAPE:
      case KEY_ESCAPE_ALT:
        showInGameMenuScreen();
        return;
    }

    // Read input for next iteration
    readCommonInput();
  }

  // Timed out, so hide all scores
  clearBelowBoard();
  centerText(HEIGHT-3,"you timed out. scoring first free row.");
  state.playerMadeMove=1;
  i=0;
  for (j=0;j<15;j++) {
    if (i==0 && state.validScores[j]>-1) {
      itoa(state.validScores[j], tempBuffer, 10);
      drawTextAlt(20-strlen(tempBuffer),scoreY[j],tempBuffer);
      i=1;
    } else if (state.validScores[j]>0) {
      drawSpace(validX,scoreY[j],3);
    }
  }

  pause(60);
}

// Invalidate state variables that will trigger re-rendering of screen items on the next cycle
void clearRenderState() {
  state.prevActivePlayer = state.prevRound = 99;
  state.prevPlayerCount = 0;
  forceReadyUpdates = true;
}

/// @brief Convenience function to draw text centered at row Y
void centerText(unsigned char y, char * text) {
  drawText(WIDTH/2-strlen(text)/2, y, text);
}

/// @brief Convenience function to draw text centered at row Y, blanking out the rest of the row
void centerTextWide(unsigned char y, char * text) {
  i = strlen(text);
  x = WIDTH/2-i/2;

  drawSpace(0,y, x);
  drawText(x, y, text);
  drawSpace(x+i,y, WIDTH-x-i);
}

/// @brief Convenience function to draw text centered at row Y in alternate color
void centerTextAlt(unsigned char y, char * text) {
  drawTextAlt(WIDTH/2-strlen(text)/2, y, text);
}

/// @brief Convenience function to draw status text centered
void centerStatusText(char * text) {
  drawTextAlt((WIDTH-strlen(text))>>1,HEIGHT-1,text);
}

/// @brief Init/reset the input field for display
/// @return 
bool resetInputField() {
  inputField_done = 1;
  disableKeySounds();
}

/// @brief Handles available key strokes for the defined input box (player name and chat). Returns true if user hits enter
bool inputFieldCycle(uint8_t x, uint8_t y, uint8_t max, char* buffer) {
  static uint8_t curx, lastY;
  
  // Initialize first call to input box
  if (inputField_done == 1 || lastY != y) {
    inputField_done=0;
    lastY=y;
    curx = strlen(buffer);
    drawTextAlt(x,y, buffer);
    drawTextcursorPos(x+curx,y);
    enableKeySounds();
  }

  // Process any waiting keystrokes
  if (kbhit()) {
    inputField_done=0;
    
    input.key = cgetc();

    // Debugging - See what key was pressed
    // itoa(input.key, tempBuffer, 10);drawText(0,0, tempBuffer);

    if (input.key == KEY_RETURN && curx>1) {
      inputField_done=1;
      // remove cursor
      drawBlank(x+curx,y);
    } else if (input.key == KEY_BACKSPACE && curx>0) {
      buffer[--curx]=0;
      drawText(x+1+curx,y," ");
    } else if (
      curx < max && ((curx>0 && input.key == KEY_SPACEBAR) || (input.key>= 48 && input.key <=57) || (input.key>= 65 && input.key <=90) || (input.key>= 97 && input.key <=122))    // 0-9 A-Z a-z
    ) {
      
      if (input.key>=65 && input.key<=90)
        input.key+=32;

      buffer[curx]=input.key;
      buffer[++curx]=0;
    }

    drawTextAlt(x,y, buffer);
    
    if (inputField_done) 
      disableKeySounds();
    else 
      drawTextcursorPos(x+curx,y);

    return inputField_done;
  }

  return false;
  
}
