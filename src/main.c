/**
 * @brief   Fujitzee
 * @author  Eric Carr, Thomas Cherryhomes, (insert names here)
 * @license gpl v. 3
 * @verbose main
 */

#ifdef _CMOC_VERSION_
#include <cmoc.h>
typedef unsigned char bool;
#define false 0
#define true 1
#else
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#endif /* _CMOC_VERSION_ */

#include "platform-specific/graphics.h"
#include "platform-specific/util.h"
#include "platform-specific/input.h"
#include "misc.h"
#include "platform-specific/sound.h"

#include "stateclient.h"
#include "gamelogic.h"
#include "screens.h"

#include "fujinet-fuji.h"

// Store default server endpoint in case lobby did not set app key
char serverEndpoint[50] = "https://fujitzee.carr-designs.com/";
char localServer[50] = "http://127.0.0.1:8080/"; // Set 3rd byte in the e41c0300 appkey to 0xff to use local

//char serverEndpoint[64] = "N:http://api.open-notify.org/iss-now.json";

char query[50] = ""; //?table=blue&player=ERICAPL2";
char playerName[12] = "";

GameState state;

// State helper vars
uint16_t rx_len, maxJifs;
bool forceReadyUpdates;

// Common local scope temp variables
unsigned char h, i, j, k, x, y;
char tempBuffer[128];
char prefs[10];

// void test(unsigned char x, unsigned char y, unsigned char z, unsigned char w) {
//   unsigned char g;
//   x=y=z=y;
//   w--;
//   if (w) {
//     test(x,y,z,w);
//   } else {
//     cputs("WAIT");
//     cgetc();
//   }
// }

#ifdef _CMOC_VERSION_
int main(void)
#else
void main(void)
#endif /* _CMOC_VERSION_ */
{
  uint8_t failedApiCalls=0;

  initGraphics(); 
  initSound();
  
  showWelcomeScreen();
  showTableSelectionScreen();
  
  // Main event loop - process state from server and input from keyboard/joystick
  state.apiCallWait=0;

  while (true) {
    
    // Poll the server every so often.
    if (!state.apiCallWait--) {

      // Housekeeping - allows platform specific housekeeping, like stopping Attract/screensaver mode in Atari
      housekeeping();

      // Poll the server
      switch (getStateFromServer()) {
        case STATE_UPDATE_ERROR:
          // ERROR - Wait a bit to avoid hammering the server if getting bad responses
          failedApiCalls++;

          // Exponential (wait an additional second each consequtive error)
          state.apiCallWait=60*failedApiCalls;

          // Don't wait more than 5 seconds
          if (state.apiCallWait>300)
            state.apiCallWait = 300;

          // After a few failures, let the player know we are experiencing technical difficulties
          if (failedApiCalls>3) {
            //drawStatusText("connection lost. reconnecting.."); 
          }
          break;
        
        case STATE_UPDATE_NOCHANGE:
          // Wait a few frames before checking for more data
          state.apiCallWait = 5;
          break;

        case STATE_UPDATE_CHANGE:
          processStateChange();
          
          // Poll again in a bit
          state.apiCallWait = 59;
          break;
      }
    }

    // Animation and input
    handleAnimation();
    processInput();
  }

#ifdef _CMOC_VERSION_
  return 0;
#endif /* CMOC_VERSION_  */
}

/*
  // May use in future for generating player id
  AdapterConfigExtended ac;
  fuji_get_adapter_config_extended(&ac);
  
  // Remove :s
  for(i=5;i>0;--i) {
    *(uint16_t*)&ac.sMacAddress[i*2] = *(uint16_t*)&ac.sMacAddress[i*3];
  }
  ac.sMacAddress[12]=0;
*/