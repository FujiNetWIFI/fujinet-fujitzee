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
uint8_t cursorPos, prevCursorPos, spectators;

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
  static uint8_t scoreCursorX, scoreCursorY, c, len,maxScoreY;
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
            sendMove("ready");
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
    for (i=1;i<7;i++) {
      for(j=0;j<15;j++)  {
        drawSpace(13+i*4, scoreY[j], 3);
      }
    }
    drawSpace(0,scoreY[15],40);
    
    centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE to toggle");
  }


  // Draw player names if the count changed
  if (true) { // (redraw || state.playerCount != state.prevPlayerCount || state.round == 0 || (state.round==1 && state.prevRound==0)) {
    spectators=0;

    for(i=1;i<=PLAYER_MAX;i++) {
      y=i+2;
      x=14+i*4;
     
      if (i<=state.playerCount) {
        
        player = &state.players[i-1]; 
        c= player->name[player->alias];
        len = (uint8_t)strlen(player->name);
        
        if (player->scores[0]==-2) {
          drawSpec(0,y);
          spectators++;
          if (i<=PLAYER_MAX) {
            drawBlank(x,1);
          }
        } else { 
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

    //if the player count changed, change the active player #, since a new player may have the same activePlayer number
    //state.prevActivePlayer = -1;
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
        if (i<state.playerCount && state.players[i].scores[0]==1) {
          drawTextVert(18+i*4,3,"ready");
          drawMark(0,i+3);
        } else {
          drawTextVert(18+i*4,3,"     ");
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
      drawBlank(17,scoreY[cursorPos-10]);
      cursorPos = 99;
    }

    if (state.round == 99) {
      drawSpace(0,HEIGHT-5,200);
      drawText(11,21,"score");
      setHighlight(-1,0,0);
    }


    // Update scores on-screen - two pass - first highlights in green the new one
    h = 0;
    scoreCursorY=0;
   
      // Skip ahead to drawing the second round if the player hasn't changed
      if (state.activePlayer == state.prevActivePlayer || redraw) {
        k=1;
      } else {
        k=0;
      }
      for (i=0;i<state.playerCount;i++) {
        // Break early if we reach spectators or go beyond 6
        if (state.players[i].scores[0]==-2 || i>6)
          break;
        h=0;
        maxScoreY= state.round<99 ? 15 : 16;
        for (j=0;j<maxScoreY;j++) {
          score = state.players[i].scores[j];
          if (score>-1) {
            itoa(score, tempBuffer, 10);
            len=(uint8_t)strlen(tempBuffer);
            if (len<3) {
              drawSpace(17+i*4,scoreY[j],len);
            }
            if (j==15 || (h==0 && k==0 && i>0 && state.activePlayer>-1 && isEmpty(19+i*4, scoreY[j]))) {
              drawTextAlt(20-len+i*4,scoreY[j],tempBuffer);
              if (scoreCursorY==0 ) {
                scoreCursorY=scoreY[j];
                scoreCursorX=17+i*4;
              }
              h=1;
            } else {
              drawText(20-len+i*4,scoreY[j],tempBuffer);
            }
          } else if (k==1) {
            // Draw blank (just in case there was something there from a previous player)
            drawSpace(17+i*4,scoreY[j],3);
          }
        }
       
      //}

      // Pause unless a ton of numbers changed
      if (!redraw && state.round<99 && scoreCursorY>0 && h) {
        drawCursor(scoreCursorX,scoreCursorY, 0);
       //  soundScoreCursor();
        pause(25);
        //for (i=0;i<2;i++) {
          drawCursor(scoreCursorX,scoreCursorY, 1); soundScore();pause(5);
          drawCursor(scoreCursorX,scoreCursorY, 0); //pause(10);
       // }
        pause(30);
        drawBlank(scoreCursorX,scoreCursorY);
      }
    }

     // Clear old turn indicator
    if (state.prevActivePlayer>-1)
      drawBlank(0,state.prevActivePlayer+3);
    
    // Draw new active player indicator
    if (state.activePlayer>-1) {
      drawMark(0,state.activePlayer+3);
      //if (state.activePlayer != 0)
      setHighlight(state.activePlayer, state.activePlayer == 0, 0);
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
      // while (!input.trigger) {
      //   readCommonInput();
      //   waitvsync();
      // }
      // drawSpace(0,HEIGHT-5,200);
     
    } else {
      if (state.activePlayer != 0)
        pause(30);
      
      drawSpace(0,HEIGHT-5,200);
      if (spectators>0) {
        itoa(spectators, tempBuffer, 10);
        strcat(tempBuffer," watching");
        drawSpec(0,HEIGHT-1);
        drawTextAlt(2,HEIGHT-1, tempBuffer);
      } 
      drawText(0,HEIGHT-4, state.prompt);
      pause(30);
    }

    if (state.activePlayer == 0 && !state.viewing) {
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
  static uint8_t highScoreIndex, isFujitzee;
  static int16_t score;

  waitvsync();  
  
  // Don't touch anything until player continues
  if (state.waitingOnEndGameContinue)
    return;

  isThisPlayer = !state.viewing && state.activePlayer==0;

  // Setup the player input details if this is a new roll
  if (state.rollsLeft != state.prevRollsLeft || state.activePlayer != state.prevActivePlayer )  {
    state.rollFrames=31;
    if (isThisPlayer) {
      state.playerMadeMove=false;
      prevCursorPos=5;
      cursorPos=1;

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
  
  // Is fujitzee eligble 
  isFujitzee = (state.activePlayer==0) && state.rollFrames == 0 && state.validScores[FUJITZEE_SCORE] == 50 ? 1 : 0;

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
      if (state.rollFrames && state.keepRoll[i]=='1' ) {
        // Draw a random die
        drawDie(20+4*i,HEIGHT-4,rand()%6+1,0,isFujitzee);
      } else {
        // Draw the kept die
        drawDie(20+4*i,HEIGHT-4,state.dice[i]-48,(state.rollFrames || state.rollsLeft>0) && state.keepRoll[i]=='0', isFujitzee);
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
          drawSpace(17,scoreY[j],3-strlen(tempBuffer)); 
        } else {
          strcpy(tempBuffer,"  ");
        }

        drawTextAlt(20-strlen(tempBuffer),scoreY[j],tempBuffer);
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
      drawSpace(0,HEIGHT-5,200);
      clearRenderState();
    }
  } else if (!state.viewing) {
    // Toggle readiness if waiting to start game
    if (state.round == 0 && input.trigger) {
      state.players[0].scores[0] = !state.players[0].scores[0];
      forceReadyUpdates=true;
      renderBoardNamesMessages();
      if (state.players[0].scores[0]) 
        soundScore();
      else
        soundScoreCursor();

      sendMove("ready");
      return;
    }

    // Wait on this player to make roll decisions
    if (!state.rollFrames && state.activePlayer == 0 && !state.playerMadeMove && state.round>0) {
      waitOnPlayerMove();
    }
  }

  switch(input.key) {
      case KEY_ESCAPE: // Esc
      case KEY_ESCAPE_ALT: // Esc Alt
        showInGameMenuScreen();  
        break;
    }    

  // static bool chatInit=false;
  // if (kbhit()) {
  //   //if (!chatInit) {
  //   //  drawText(20,23,">>");
  //   //  inputFieldCycle(22,23,18, chat);
  //     //chat[0]=0;
  //     //chatInit=true;
  //   //}

    
  // }
  //readCommonInput();
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
      
      // Update vertical cursorPos location - scores: 10-24 represent the possible scoring locations
      } else if (input.dirY != 0) {
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
        hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+16, HEIGHT-4);
      else {
        drawBlank(17,scoreY[prevCursorPos-10]);
        if (state.validScores[prevCursorPos-10]==0) {
          drawBlank(19,scoreY[prevCursorPos-10]);
        }
      }

      // Draw cursorPos
      if (cursorPos < 6) {
        drawDiceCursor(4*cursorPos-(cursorPos==0)+16, HEIGHT-4);
        soundCursor();
      } else {
        drawCursor(17,scoreY[cursorPos-10],0);
        if (state.validScores[cursorPos-10]==0) {
          drawTextAlt(19,scoreY[cursorPos-10],"0");
        }
        soundScoreCursor();
      }
      prevCursorPos = cursorPos;
      
      
    } else if (cursorPos>9){ 
      drawCursor(17,scoreY[cursorPos-10],(frames<3)*0x80);
    }

    // Handle trigger press
    if (input.trigger) {
      
      if (cursorPos>=10) {
        // Select score
        i=cursorPos-10;

        // Cursor select animation 
        drawCursor(17,scoreY[cursorPos-10],1);soundScore();
        
        // First, hide all scores but the main score
        for (j=0;j<15;j++) {
          if (state.validScores[j]>0) {
            if (j != i)
              drawSpace(17,scoreY[j],3);
          }
        }

        pause(5);drawCursor(17,scoreY[cursorPos-10],0);; pause(10);

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
        drawDie(16+4*cursorPos,HEIGHT-4,state.dice[cursorPos-1]-48,i == '0', 0);
        if (i=='0')
          soundKeep();
        else 
          soundRelease();
      } else {
        // Request another roll

        // Highlight the roll die in green
        drawDie(15,HEIGHT-4,state.rollsLeft+15,0,0);
        
        strcpy(tempBuffer, "roll/");
        strcat(tempBuffer, state.keepRoll);
        sendMove(tempBuffer);

        state.playerMadeMove = true;
        hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+16, HEIGHT-4);
        // Clear clock
        drawSpace(6,HEIGHT-3,3);
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
        drawTextAlt(8-strlen(tempBuffer), HEIGHT-3, tempBuffer);
        drawClock(8,HEIGHT-3);
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
  drawSpace(0,HEIGHT-5,200);
  centerText(HEIGHT-3,"you timed out. scoring first free row.");
  state.playerMadeMove=1;
  i=0;
  for (j=0;j<15;j++) {
    if (i==0 && state.validScores[j]>-1) {
      itoa(state.validScores[j], tempBuffer, 10);
      drawTextAlt(20-strlen(tempBuffer),scoreY[j],tempBuffer);
      i=1;
    } else if (state.validScores[j]>0) {
      drawSpace(17,scoreY[j],3);
    }
  }

  pause(60);
}

// Invalidate state variables that will trigger re-rendering of screen items on the next cycle
void clearRenderState() {
  state.prevRound = 99;
  state.prevPlayerCount = 0;
  state.prevActivePlayer = 99;
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


/// @brief Handles available key strokes for the defined input box (player name and chat). Returns true if user hits enter
bool inputFieldCycle(uint8_t x, uint8_t y, uint8_t max, char* buffer) {
  static uint8_t done, curx, lastY;
  
  // Initialize first call to input box
  //i=strlen(buffer);
  if (done == 1 || lastY != y) {
    done=0;
    lastY=y;
    curx = strlen(buffer);
    drawTextAlt(x,y, buffer);
    drawTextcursorPos(x+curx,y);
    enableKeySounds();
  }
   // curx=i;
   // done=0;
 // }




  // Process any waiting keystrokes
  if (kbhit()) {
    done=0;
    
    input.key = cgetc();

    // Debugging - See what key was pressed
    // itoa(input.key, tempBuffer, 10);drawText(0,0, tempBuffer);

    if (input.key == KEY_RETURN && curx>1) {
      done=1;
      // remove cursorPos
      drawText(x+1+i,y," ");
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
    drawTextcursorPos(x+curx,y);

    if (done==1) 
      disableKeySounds();

    return done==1;
  }

  return false;
  
}
