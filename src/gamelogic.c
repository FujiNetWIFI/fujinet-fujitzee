#include <stdlib.h>
#include <string.h>
#include "platform-specific/graphics.h"
#include "platform-specific/sound.h"
#include "platform-specific/util.h"
#include "gamelogic.h"
#include "misc.h"
#include "stateclient.h"
#include "screens.h"
#include <peekpoke.h>

#define PLAYER_LIST_Y_OFFSET 5
#define BOTTOM_PANEL_Y HEIGHT-BOTTOM_HEIGHT
#define HELP_X WIDTH-18

uint8_t chat[20]=""; 
uint8_t scoreY[] =   {3,4,5,6,7,8,10,11,13,14,15,16,17,18,19,21};
char* scores[]={"one","two","three","four","five","six","total","bonus","set 3","set 4","house","s run","l run","count"};
uint8_t cursorPos, prevCursorPos, spectators, inputField_done, validX;
bool currentlyShowingHelp = 0;

void progressAnim(unsigned char y) {
  static uint8_t i;
  
  for(i=0;i<3;++i) {
    pause(10);
    drawIcon(WIDTH/2-2+i*2,y, ICON_MARK);
  }
}

void processStateChange() {

  renderBoardNamesMessages();
  handleAnimation();

  state.prevPlayerCount=clientState.game.playerCount;
  state.prevActivePlayer = clientState.game.activePlayer;
  state.prevRollsLeft = clientState.game.rollsLeft;
  state.prevRound = clientState.game.round;
}

void clearScores(uint8_t x) {
  for(j=0;j<15;j++)  {
    drawSpace(x, scoreY[j], 3);
  }
}

#define READY_LEFT WIDTH/2-8

void renderBoardNamesMessages() {
  static bool redraw, fullWidth;
  static uint8_t scoreCursorX, scoreCursorY, c, len,maxScoreY, newScoreFound, ignoreNewScore, mostRecentPlayer, i4;
  static Player *player; 
  static int16_t score;
  
  // Could move this to a define to save space if runtime check is not needed
  fullWidth = WIDTH>=40;

  // If player is waiting on end game screen, auto-ready up if the game is starting
  
  // Redraw the entire board when going back to round 0 (ready up)
  redraw = clientState.game.round < state.prevRound || clientState.game.round == 1 && state.prevRound==0;

  if (state.waitingOnEndGameContinue) {
    if (clientState.game.round == 1) {
      state.waitingOnEndGameContinue =false;
      // Force a redraw since the player waited on the end game screen until start
      redraw = true;
      clearRenderState();
    } else {
      
      if (state.countdownStarted)
        centerTextWide(HEIGHT-3,clientState.game.prompt);

      if (!state.countdownStarted && clientState.game.prompt[0]== 's') {
          soundJoinGame();
          if (clientState.game.players[0].scores[0] < 1) {
            apiCallForAll("ready");
          }
          
          state.countdownStarted = true;
        } else if (state.countdownStarted && clientState.game.prompt[0]!= 's') {
          state.countdownStarted = false;
        }
      return;
    }
  }
  

  if (redraw || state.drawBoard) {
    resetScreenNoBorder();
    redraw=true;

    if (clientState.game.round>0) {
      // Round 1+
      drawBoard();
      if (fullWidth) {
        drawLogo(0,0);
        drawTextAlt(1,4,"players");
      }
      
    } else {
      // Round 0 - Ready Up screen
      drawLogo(WIDTH/2-5,1);
      centerTextAlt(6, clientState.game.serverName);

      drawLine(READY_LEFT,7,16);
      centerTextAlt(18,"press ESC for menu");  
      centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE to toggle");
    }
  
   
    state.drawBoard = false;
  }
  

  // Draw player names
  spectators=0;
  if (clientState.game.round>0) {
    for(i=1;i<=PLAYER_MAX;i++) {
      y=i+PLAYER_LIST_Y_OFFSET;
      x=SCORES_X+3+i*4;
      
      if (i<=clientState.game.playerCount) {
        
        player = &clientState.game.players[i-1]; 
        c= player->name[player->alias];
        len = (uint8_t)strlen(player->name);
        
        if (player->scores[0]==-2) {
          // Draw spec icon in front of name 
          if (fullWidth)
            drawIcon(0,y, ICON_SPEC); 
            
          spectators++;
          // Clear initial/scoreboard for this player index if they were not previously viewing
          if (i<7 && !state.isViewing[i]) {
            drawBlank(x,1);
            clearScores(x-1);
            
            state.isViewing[i]=true;
          }
        } else { 
          state.isViewing[i]=false;
          // Clear player indicator
          if (clientState.game.round>0 && clientState.game.activePlayer != i-1) {
            if (fullWidth)
              drawBlank(0,y);
            else if (!currentlyShowingHelp)
              drawBlank(x-1,1);
          } 
          
          // Player initials across top of screen
          if (!currentlyShowingHelp)
            drawChar(x,1,c,1);
        }
        
        // Draw player's name, highlighting the alias as green
        if (fullWidth) {
          for (j=0;j<player->alias;j++) {
            drawChar(1+j,y,player->name[j], 0);
          }
          drawChar(1+j,y,player->name[j], 1);
          drawText(2+player->alias,y,player->name+player->alias+1);
          if (len<8) {
            drawSpace(1+len, y, 8-len);
          }
        }

      } else if (i<=state.prevPlayerCount || state.prevPlayerCount==0) {
        // Blank out entries for this player
        if (fullWidth)
          drawSpace(0,y,9);
        
        // Blank scoreboard
        if (i<7) {
          drawBlank(x,1);
          clearScores(x-1);
        }
      }
    }
  }

  // Round 0 (waiting to start) checks, or going into round 1
  if (clientState.game.round ==0) { //} || (clientState.game.round == 1 && state.prevRound==0)) {
    // Display "waiting for players" prompt if changed in ready mode
    if (clientState.game.round==0) {
        centerTextWide(HEIGHT-3,clientState.game.prompt);
        
      if (clientState.game.prompt[0]== 's') {
        if (state.countdownStarted)
          soundTick();
        else {
          soundJoinGame();
          state.countdownStarted = true;
        }
      } else {
        state.countdownStarted = false;
      }
    }

    // Show players that are ready to start  
    c++;
    for(i=0;i<9;i++) {
      if (i<clientState.game.playerCount) {
        drawText(READY_LEFT, 8+i, clientState.game.players[i].name);
        len = (uint8_t)strlen(clientState.game.players[i].name);
        if (len<8) {
          drawSpace(READY_LEFT+len, 8+i, 8-len);
        }
        drawText(READY_LEFT, 8+i, clientState.game.players[i].name);
        if (clientState.game.players[i].scores[0]) { 
          drawTextAlt(READY_LEFT+11,8+i,"ready");
          //drawIcon(READY_LEFT+10,8+i, ICON_MARK);
        } else {
          drawSpace(READY_LEFT+11, 8+i, 5);
          drawIcon(READY_LEFT+12+(c%3),8+i, ICON_MARK);
        }
      } else if (i< state.prevPlayerCount) {
        drawSpace(READY_LEFT, 8+i, 16);
      }
    }

    // Exit early as below text is for rounds > 0 
    return;
  }


  // Scoreboard and prompt - refresh if the active player changed, or round changed
  if (clientState.game.activePlayer != state.prevActivePlayer || clientState.game.round != state.prevRound || clientState.game.playerCount != state.prevPlayerCount) {
    
    // Remove score cursorPos
    if (cursorPos>10 && cursorPos<99) {
      drawBlank(validX,scoreY[cursorPos-10]);
      cursorPos = 99;
    }

    if (clientState.game.round == 99) {
      clearBelowBoard();

      // Only specify scores header if there are scores (game not aborted early)
      if (clientState.game.players[0].scores[15]>0) {
        drawText(SCORES_X,21,"score");
      }
      setHighlight(-1,0,0);
    }


    // Update scores on-screen
    newScoreFound = 0;
    scoreCursorY=0;
   
    // Skip ahead to drawing the second round if the player hasn't changed
    ignoreNewScore= clientState.game.activePlayer == state.prevActivePlayer || redraw;
    

    // In case this client is lagging behind (multiple players scored since)
    // only animate the most recent player's score
    mostRecentPlayer = (clientState.game.activePlayer+clientState.game.playerCount-1) % clientState.game.playerCount;
    while (clientState.game.players[mostRecentPlayer].scores[0]==-2) {
      mostRecentPlayer = (mostRecentPlayer+clientState.game.playerCount-1) % clientState.game.playerCount;
    }


    for (i=clientState.game.playerCount-1;i<255;i--) {
      // Skip spectators or going beyond 6 players
      if (i>5 || clientState.game.players[i].scores[0]==-2 || (currentlyShowingHelp && i>0))
        continue;
      h=SCORES_X+6+i*4;
      i4=h+3;
      maxScoreY= clientState.game.round<99 ? 15 : 16;
      for (j=0;j<maxScoreY;j++) {
        score = clientState.game.players[i].scores[j];
        if (score>-1) {
          itoa(score, tempBuffer, 10);
          len=(uint8_t)strlen(tempBuffer);
          if (len<3) {
            drawSpace(h,scoreY[j],len);
          }
          if (j!=6 && j!=7 && j!=15 && !newScoreFound && !ignoreNewScore && i!= state.localPlayer[state.currentLocalPlayer].index && i==mostRecentPlayer && !state.renderedScore[i*16+j]) {
            drawTextAlt(i4-len,scoreY[j],tempBuffer);
            if (scoreCursorY==0 ) {
              scoreCursorY=scoreY[j];
              scoreCursorX=h;
            }
            newScoreFound=1;
          } else {
            drawText(i4-len,scoreY[j],tempBuffer);
          }
          state.renderedScore[i*16+j]=true;
        } else if (ignoreNewScore) {
          // Draw blank (just in case there was something there from a previous player)
          drawSpace(h,scoreY[j],3);
        }
      }
    }

    // Animate arrow showing newly added score
    if (!redraw && clientState.game.round<99 && scoreCursorY>0 && newScoreFound) {
      drawIcon(scoreCursorX,scoreCursorY, ICON_CURSOR);
      pause(25);
      drawIcon(scoreCursorX,scoreCursorY, ICON_CURSOR_ALT);
      soundScore();
      pause(5);
      drawIcon(scoreCursorX,scoreCursorY, ICON_CURSOR);
      pause(30);
      drawBlank(scoreCursorX,scoreCursorY);
    }


     // Clear old turn indicator
    if (fullWidth && state.prevActivePlayer>-1)
      drawBlank(0,state.prevActivePlayer+PLAYER_LIST_Y_OFFSET+1);
    
    // Draw active player indicator
    if (clientState.game.activePlayer>-1) {
      if (fullWidth)
        drawIcon(0,clientState.game.activePlayer+PLAYER_LIST_Y_OFFSET+1, ICON_MARK);
      else
        drawIcon(SCORES_X+6+clientState.game.activePlayer*4,1, ICON_MARK);
        
      setHighlight(clientState.game.activePlayer, state.localPlayerIsActive, 0);
    } else {
      setHighlight(-1,0,0);
    }
    

    // Display current round
    if (clientState.game.round != state.prevRound) {
      if (clientState.game.round>0 && clientState.game.round< 14) {
        // Display static round details
        itoa(clientState.game.round, tempBuffer, 10);
        if (fullWidth) {
          drawTextAlt(1,3,"round ");
          drawTextAlt(1,4,"   of ");
          drawText(7,4,"13"); 
          drawText(8-(clientState.game.round>9),3,tempBuffer);
        } else {
          drawText(3-(clientState.game.round>9),1,tempBuffer);
          drawTextAlt(4,1,"/");
          drawText(5,1,"13");
          
        }
      } else if (fullWidth) {
        drawSpace(1,3,8);
        drawTextAlt(1,4,"players ");
      }
    }

   
    if (clientState.game.round < 99) { 
      // Normal game messaging
      if (!state.localPlayerIsActive)
        pause(30);
      
      clearBelowBoard();
      if (spectators>0) {
        itoa(spectators, tempBuffer, 10);
        
        
        if (fullWidth) {
          drawIcon(0,HEIGHT-1, ICON_SPEC);
          strcat(tempBuffer," watching");
        } else {
          strcat(tempBuffer," spec");
          if (spectators>1)
            strcat(tempBuffer,"s");
        }
        drawTextAlt(1+fullWidth,HEIGHT-1, tempBuffer);
      } 

      // Visual override - when playing multiple local players, put player's name + your turn
      if (state.localPlayerIsActive) {
        if (fullWidth) {
        drawText(0,BOTTOM_PANEL_Y,"your turn");
        drawText(0,BOTTOM_PANEL_Y+1,clientState.game.players[clientState.game.activePlayer].name);
        } else {
          drawText(1,BOTTOM_PANEL_Y,"your");
          drawText(1,BOTTOM_PANEL_Y+1,"turn");
          //drawText(0,BOTTOM_PANEL_Y+2,clientState.game.players[clientState.game.activePlayer].name); 
        }
      } else {
        if (fullWidth) {
          drawText(0,BOTTOM_PANEL_Y, clientState.game.prompt); 
        } else {
          drawText(1,BOTTOM_PANEL_Y,"waiting on");
          drawText(1,BOTTOM_PANEL_Y+1,clientState.game.players[clientState.game.activePlayer].name);
          
        }
      }
      
      pause(30);
      
    } else {
       // Handle end of game

      // Clear active indicator
      if (fullWidth && state.prevActivePlayer>-1) {
        drawBlank(0,state.prevActivePlayer+PLAYER_LIST_Y_OFFSET+1);
      }

      centerText(GAMEOVER_PROMPT_Y, clientState.game.prompt);

      if (clientState.game.round != state.prevRound) {
        soundGameDone();

        centerTextAlt(HEIGHT-1, "please wait..");
        pause(240);
        centerTextAlt(HEIGHT-1,"press TRIGGER/SPACE to continue");
        state.waitingOnEndGameContinue = true;
        state.countdownStarted = false;
      } 
      clearCommonInput();     
    }

    if (state.localPlayerIsActive && !clientState.game.viewing) {
      setHighlight(clientState.game.activePlayer, true, 2);
      pause(5);
      soundMyTurn();
      pause(5);
      setHighlight(clientState.game.activePlayer, true, 0);
    }
  }
}

void handleAnimation() {
  static bool isThisPlayer;
  static uint8_t highScoreIndex, isFujitzee, i4, keptCountChanged;
  static int16_t score;

  waitvsync();  
  
  // Don't touch anything until player continues
  if (state.waitingOnEndGameContinue)
    return;

  isThisPlayer = !clientState.game.viewing && state.localPlayerIsActive;

  // Setup the player input details if this is a new roll
  if (clientState.game.rollsLeft != state.prevRollsLeft || clientState.game.activePlayer != state.prevActivePlayer )  {
    state.rollFrames=ROLL_FRAMES;
    
    if (clientState.game.rollsLeft==2) {
      strcpy(state.prevKept,"11111");
      strcpy(state.prevDice,clientState.game.dice);
    }

    if (isThisPlayer) {
      state.playerMadeMove=false;
      prevCursorPos=5;
      cursorPos=1;
      validX=SCORES_X+6+clientState.game.activePlayer*4;


      // If no rolls are remaining, default the cursorPos on the highest score
      if (clientState.game.rollsLeft==0) {
        highScoreIndex = 0;
        for (j=1;j<15;j++) {
          if (clientState.game.validScores[j]>clientState.game.validScores[highScoreIndex])
            highScoreIndex = j;
        }
        cursorPos=10+highScoreIndex;
      }
    }
  }

  if (clientState.game.activePlayer<0 || !state.rollFrames)
    return;

  state.rollFrames--;
  
  // Check for Fujitzee roll (all 5 dice match each other)
  // To display special animation
  if (state.rollFrames==0) {
    strcpy(state.prevDice,clientState.game.dice);
    for (i=1;i<5;i++) {
      if (clientState.game.dice[i]!=clientState.game.dice[0])
        break;
    }
    isFujitzee=i==5;
  } else {
    isFujitzee=0;
  }
  
  // Draw the dice, randomly displaying the ones that are currently being rolled
  if (state.rollFrames % ROLL_SOUND_MOD ==0)
    soundRollDice();


  keptCountChanged=state.rollFrames == ROLL_FRAMES-1 && !isThisPlayer && strcmp(state.prevKept, clientState.game.keepRoll);

  for(j=0;j<=isFujitzee;j++) {

    // Play fujitzee sound when rolled, then clear flag so dice return to white
    if (isFujitzee && j) {
      soundFujitzee();
      isFujitzee=0;
    }
    for(i=0;i<5;i++) {
      i4=WIDTH-20+4*i;
      if (state.rollFrames && clientState.game.keepRoll[i]=='1' ) {
        // Draw a random die after the first frame
        drawDie(i4,HEIGHT-4,state.rollFrames < ROLL_FRAMES-1 ? rand()%6+1 : state.prevDice[i]-48,0,isFujitzee);
      } else {
        // Draw the kept die
        drawDie(i4,HEIGHT-4,clientState.game.dice[i]-48,(state.rollFrames || clientState.game.rollsLeft>0) && clientState.game.keepRoll[i]=='0', isFujitzee);
      }
    }
    
    soundStop();

    // Pause a bit on the first render of a roll to show kept dice
    if (keptCountChanged) {
      strcpy(state.prevKept,clientState.game.keepRoll);
      pause(30);
    }
  }


  // If the rolling has stopped and this player is playing
  if (isThisPlayer && !state.rollFrames) {

    // Display the potential scores for this player
    for (j=0;j<15;j++) {
      score= clientState.game.validScores[j];
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
    drawDie(ROLL_X,HEIGHT-4,clientState.game.rollsLeft+13,0,0);
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
  } else if (!clientState.game.viewing) {
    // Toggle readiness if waiting to start game
    if (clientState.game.round == 0 && input.trigger) {
      j=!clientState.game.players[0].scores[0];
      for(i=0;i<prefs.localPlayerCount;i++) {
        clientState.game.players[state.localPlayer[i].index].scores[0] = j;
      }
      renderBoardNamesMessages();
      if (clientState.game.players[0].scores[0]) 
        soundScore();
      else
        soundScoreCursor();

      apiCallForAll("ready");
      return;
    }

    // Wait on this player to make roll decisions
    if (!state.rollFrames && state.localPlayerIsActive && !state.playerMadeMove && clientState.game.round>0) {
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

void hideInGameHelp() {
  if (!currentlyShowingHelp)
    return;

  if (!restoreScreen()) {
    state.drawBoard = true;
   // clearRenderState();
   // setHighlight(-1,0,0);
   // processStateChange();
  }
  currentlyShowingHelp = false;
}

void showInGameHelp() {
  if (clientState.game.activePlayer!=0)
    return;

  currentlyShowingHelp=true;
  saveScreen();
  
  for(y=1;y<20 ;y++) {
    drawSpace(SCORES_X+10,y,WIDTH-SCORES_X-10);
  }
  
  drawBox(HELP_X-1,1,17, 17);
  
  drawText(HELP_X+2, 3,"it's your turn");
                                                  //123456789012345678
  drawText(HELP_X, 5,"move up/down to");
  drawText(HELP_X, 6,"choose a score on");
  drawText(HELP_X, 7,"the left based on");
  drawText(HELP_X, 8,"your dice.");

  drawText(HELP_X,10,"before you score,");
  drawText(HELP_X,11,"you may re-roll ");
  drawText(HELP_X,12,"any of your dice");
  drawText(HELP_X,13,"up to two times.");

  drawText(HELP_X,15,"move left/right,");
  drawText(HELP_X,16,"and pick dice to");
  drawText(HELP_X,17,"keep, then roll.");
}

void waitOnPlayerMove() {
  static bool foundValidLocation;
  static uint8_t waitCount, frames; 
  static uint16_t jifsPerSecond;
  resetTimer();

  // Determine max jiffies for PAL and NTS
  jifsPerSecond=getJiffiesPerSecond();
  maxJifs = jifsPerSecond*clientState.game.moveTime;
  waitCount=frames=0;
  
  // If this is the player's first time playing, show instructions
  if (!prefs.hasPlayed) {
    prefs.hasPlayed=true;
    showInGameHelp();
    savePrefs();
  }

  // Move selection loop
  while (clientState.game.moveTime>0) {
    frames=(frames+1) % 30;
    waitvsync();

    // Handle trigger press
    if (input.trigger) {
      
      if (cursorPos>=10) {
        // Select score
        i=cursorPos-10;

        state.renderedScore[clientState.game.activePlayer*16+i]=true;

        // Cursor select animation 
        drawIcon(validX,scoreY[cursorPos-10],ICON_CURSOR_ALT);soundScore();
        
        // First, hide all score options but the chosen score
        for (j=0;j<15;j++) {
          if (clientState.game.validScores[j]>0) {
            if (j != i)
              drawSpace(validX,scoreY[j],3);
          }
        }

        pause(5);drawIcon(validX,scoreY[cursorPos-10],ICON_CURSOR);; pause(10);

        // Send command to score this value
        strcpy(tempBuffer, "score/");
        itoa(i, tempBuffer+6, 10);
        sendMove(tempBuffer);

        hideInGameHelp();
        clearBelowBoard();
        state.playerMadeMove = true;

        return;
      } else if (cursorPos>0) {
        // Toggle kept state of die
        i = clientState.game.keepRoll[cursorPos-1]= clientState.game.keepRoll[cursorPos-1]=='1' ? '0' : '1';
        drawDie(WIDTH-24+cursorPos*4,HEIGHT-4,clientState.game.dice[cursorPos-1]-48,i == '0', 0);
        if (i=='0')
          soundKeep();
        else 
          soundRelease();
        
        drawDie(ROLL_X,HEIGHT-4,13+ (strcmp(clientState.game.keepRoll,"00000") ? clientState.game.rollsLeft : 3),0,0);
      } else {
        // Request another roll, assuming at least one dice is not kept
        if (strcmp(clientState.game.keepRoll,"00000")) {
          // Highlight the roll die in green
          drawDie(ROLL_X,HEIGHT-4,clientState.game.rollsLeft+13,0,1);
          soundRollButton();
          
          strcpy(tempBuffer, "roll/");
          strcat(tempBuffer, clientState.game.keepRoll);
          sendMove(tempBuffer);

          state.playerMadeMove = true;
          hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+WIDTH-24);
          // Clear clock
          drawSpace(TIMER_X,BOTTOM_PANEL_Y,2);
          drawSpace(TIMER_X+TIMER_NUM_OFFSET_X-2,BOTTOM_PANEL_Y+TIMER_NUM_OFFSET_Y,2);
          return;
        } else {
          // If all 5 dice are kept, there is nothing to roll. 
          // Flash all dice, then move cursor up
          for (j=1;j<255;j--) {
            drawDie(ROLL_X,HEIGHT-4,16,0,j);
            for(i=0;i<6;i++) {
              drawDie(WIDTH-20+4*i,HEIGHT-4,clientState.game.dice[i]-48,1,j);
            }
            soundRelease();
            pause(6);
          }
          
          input.dirY=-1;
          input.dirX=0;
        }
      }
    }
      
    // Update horizontal cursorPos location - dice: 0 = roll, 1-5 equal select dice at that index
    if (input.dirX !=0 && clientState.game.rollsLeft) {
      // If cursorPos was selecting a score, bring it back down
      if (cursorPos>9)
        cursorPos = 1;

      cursorPos+=input.dirX;

      // Bounds check
      if (cursorPos>5)
        cursorPos= cursorPos==255?5:0;
      
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
        if (cursorPos<10 || (cursorPos>24 && clientState.game.rollsLeft==0))
          cursorPos = prevCursorPos;
        else if (cursorPos>24)
          cursorPos=prevCursorPos <10 ? prevCursorPos : 1;
        else {
          // Skip over invalid scores
          if (clientState.game.validScores[cursorPos-10]<0)
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
        hideDiceCursor(4*prevCursorPos-(prevCursorPos==0)+WIDTH-24);
      else {
        h=prevCursorPos-10;
        drawBlank(validX,scoreY[h]);
        if (clientState.game.validScores[h]==0) {
          drawBlank(validX+2,scoreY[h]);
        }
      }

      // Draw cursorPos
      if (cursorPos < 6) {
        drawDiceCursor(4*cursorPos-(cursorPos==0)+WIDTH-24);
        soundCursor();
      } else {
        h=cursorPos-10;
        drawIcon(validX,scoreY[h],ICON_CURSOR);
        if (clientState.game.validScores[h]==0) {
          drawTextAlt(validX+2,scoreY[h],"0");
        }
        soundScoreCursor();
      }
      prevCursorPos = cursorPos;
      
      
    } else if (cursorPos>9){ 
      drawIcon(validX,scoreY[cursorPos-10],(frames<2)? ICON_CURSOR_BLIP : ICON_CURSOR);
    }
    
    // Tick counter once per second   
    if (++waitCount>5) {
      waitCount=0;
      i = (maxJifs-getTime())/jifsPerSecond;
      if (i<=15 && i != clientState.game.moveTime) {
        clientState.game.moveTime = i;
        if (i<10)
          tempBuffer[0]=' ';
          
        itoa(i, tempBuffer+(i<10), 10);
        drawTextAlt(TIMER_X+TIMER_NUM_OFFSET_X-strlen(tempBuffer), BOTTOM_PANEL_Y+TIMER_NUM_OFFSET_Y, tempBuffer);
        drawClock(TIMER_X,BOTTOM_PANEL_Y);
        soundTick();
      }
    } 

    // Pressed Esc
    switch (input.key) { 
      case 'h':case 'H':
        if (currentlyShowingHelp)
          hideInGameHelp();
        else
          showInGameHelp();
        break;
      case KEY_ESCAPE:
      case KEY_ESCAPE_ALT:
        hideInGameHelp();
        showInGameMenuScreen();
        return;
    }

    // Read input for next iteration
    readCommonInput();
  }

  // Timed out, so hide all scores
  hideInGameHelp();
  clearBelowBoard();
  
  centerText(BOTTOM_PANEL_Y+1,"you timed out. auto scoring.");
  state.playerMadeMove=1;
  i=0;
  for (j=0;j<15;j++) {
    if (clientState.game.validScores[j]>-1) {
      if (!i) {
        itoa(clientState.game.validScores[j], tempBuffer, 10);
        drawTextAlt(validX+3-strlen(tempBuffer),scoreY[j],tempBuffer);
        i=1;
      } else {
        drawSpace(validX,scoreY[j],3);
      }
    }
  }

  pause(60);
}

// Invalidate state variables that will trigger re-rendering of screen items on the next cycle
void clearRenderState() {
  state.prevActivePlayer = state.prevRound = 99;
  state.prevPlayerCount = 0;
  memset(state.renderedScore, 0, 16*6);
  state.drawBoard=true;
  setHighlight(-1,0,0);
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
void resetInputField() {
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
    drawIcon(x+curx,y, ICON_TEXT_CURSOR);
    enableKeySounds();
  }

  // Process any waiting keystrokes
  if (kbhit()) {
    inputField_done=0;
    
    input.key = cgetc();

    // Debugging - See what key was pressed
    //itoa(input.key, tempBuffer, 10);drawText(0,0, tempBuffer);

    if (input.key == KEY_RETURN && curx>1) {
      inputField_done=1;
      // remove cursor
      drawBlank(x+curx,y);
    } else if ((input.key == KEY_BACKSPACE || input.key == KEY_LEFT_ARROW) && curx>0) {
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
      drawIcon(x+curx,y, ICON_TEXT_CURSOR);

    return inputField_done;
  }

  return false;
  
}
