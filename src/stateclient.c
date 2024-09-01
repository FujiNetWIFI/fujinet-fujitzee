#ifdef _CMOC_VERSION_
#include <cmoc.h>
#else
#include <string.h>
#endif /* CMOC_VERSION */
#include <stdlib.h>
#define POKE(addr,val)     (*(unsigned char*) (addr) = (val))
#define POKEW(addr,val)    (*(unsigned*) (addr) = (val))
#define PEEK(addr)         (*(unsigned char*) (addr))
#define PEEKW(addr)        (*(unsigned*) (addr))
#include<stdio.h>
#include "stateclient.h"
#include "misc.h"
#include "fujinet-network.h"

char rx_buf[800];     // buffer for payload

// Internal to this file
static char url[160];
//static char hash[32] = ""; 
//static uint16_t rx_pos=0;
char *requestedMove;

void updateState(bool isTables) {
  static char *line, *nextLine, *end, *key, *value, *parent, *arrayPart;
  static bool isKey, inArray;
  static char c;
  static unsigned int i;

  // Reset state and vars
  isKey=true; inArray=false;
  state.playerCount=state.tableCount=state.currentLocalPlayer=state.localPlayerIsActive=0;
  
  parent = NULL;

  // Load state by looping through result and extracting each string at each EOL character
  end = rx_buf + rx_len;
   
  // Ensure buffer ends with string terminator
  rx_buf[rx_len]=0;

  line = rx_buf;

  while (line < end) {
    // Capture next line position, in case the current line is shortened 
    // in process of reading
    nextLine=line+strlen(line)+1;
    if (isKey) {
      key = line;

      // Special case - "pl" (players) keys are arrays of key/value pairs
      if (strcmp(key,"pl")==0 || strcmp(key,"null")==0) {

        // If the key is a NULL object, we effectively break out of the array by setting parent to empty
        if (strcmp(key,"null")==0)
          key="";
        
        parent=key;
 
        // Reset isKey since the next line will be a key
        isKey = false;
      } 
    } else {
      value = line;
      if (value[0]==0)
        value = "";

   
      // Set our state variables based on the key
      if (isTables) {
        switch (key[0]) {
          case 't': 
            state.tables[state.tableCount].table = value;
            break;
          case 'n': 
            state.tables[state.tableCount].name = value; 
            break;
          case 'p': 
            strcpy(state.tables[state.tableCount].players, value); 
            break;
          case 'm': 
            strcat(state.tables[state.tableCount].players, " / ");
            strcat(state.tables[state.tableCount].players, value); 
            state.tableCount++;
            break;
          default:
           break;
        }
      } else if (parent[0]=='p') { 
        switch (key[0]) {
          case 'n': 
            // Cap name at 8 chars max
            if (strlen(value)>8) 
              value[8]=0; 
              state.players[state.playerCount].name=value;
              
              // Map this player to a local player
              for(i=0;i<prefs.localPlayerCount;i++) {
                if (strcmp(value, prefs.localPlayer[i].name)==0) {
                  state.localPlayer[i].index = state.playerCount;
                  // If this player is the active player, set the appropriate local player index and flag it is a local player's turn
                  if (state.playerCount == state.activePlayer) {
                    state.currentLocalPlayer = i;
                    state.localPlayerIsActive = true;
                  }
                  break;
                }
              }
            
            break;
          case 'a':
            state.players[state.playerCount].alias = atoi(value);
            break;
          case 's':
            arrayPart = strtok(value, ",");
            for(i = 0; arrayPart != NULL && i<16; i++) {
                state.players[state.playerCount].scores[i] = atoi(arrayPart);
                arrayPart = strtok(NULL, ",");
            }

            // Scores is the last property, so increase the player counter
            state.playerCount++;
            forceReadyUpdates=true;
            break;
          default:
            parent="";
        }
      } else {
        switch (key[0]) {
          //case 'h':
          //  strcpy(hash, value);
          //  break;
          case 'n':
            if (strlen(value)>0) {
              strcpy(state.serverName, value); 
            }
            break;
          case 'p':
            //if (strcmp(value, state.prompt)!=0)
              state.promptChanged = true;
            state.prompt = value;
            break;
          case 'r' :
            state.round = atoi(value);
            break;
          case 'l':
            state.rollsLeft = atoi(value);
            break;
          case 'a':
            state.activePlayer = atoi(value);
            break;
           case 'm':
            state.moveTime = atoi(value);
            break;
          case 'v': 
            state.viewing = value[0]=='1';
            break;
          case 'd':
            state.dice = value;
            break;
          case 'k':
            state.keepRoll = value;
            break;
          case 'c':
            arrayPart = strtok(value, ",");
            for(i = 0; arrayPart != NULL && i<15; i++) {
                state.validScores[i] = atoi(arrayPart);
                arrayPart = strtok(NULL,  ",");
            }
            break;
        } 
      }
      
    }
  
    isKey = !isKey;
    line=nextLine;
  }  
 
}

/*
 * @brief Makes an Api call, returning true if valid payload received
 * Returns API_CALL_*:
 *  1 - successfully received a payload
 *  2 - async - received payload, still in process, call for more data
 *  0 - error - aborted
*/
uint8_t apiCall(char *path) {
  static int16_t n;
  static uint8_t* buf;
  static char *query;

  strcpy(url, "n:");
  strcat(url, serverEndpoint);
  strcat(url, path);
  query = &state.localPlayer[state.currentLocalPlayer].query;
  strcat(url, query );
  strcat(url, query[0] ? "&raw=1&lc=1" : "?raw=1&lc=1");
  
  // Initialize start of buffer for async calls, and reset position
  buf = rx_buf;

  // Network error. Reset position and abort.
  if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE)) {
    return API_CALL_ERROR;
  }

  n = network_read(url, rx_buf, sizeof(rx_buf));
  network_close(url);

  if (n<=0) {
    rx_len=0;
    return API_CALL_ERROR;
  }

  rx_len=n;
  return API_CALL_SUCCESS;
}

void sendMove(char* move) {
  if (move != NULL)
    state.apiCallWait=0;

  requestedMove = move;
}

/// @brief Makes the requested call for all local players. Suitable for joining, ready toggle, leaving
/// @param path 
void apiCallForAll(char* path ) {
  for(i=0;i<prefs.localPlayerCount;i++) {
    state.currentLocalPlayer = i;
    apiCall(path);
  }
}

uint8_t getStateFromServer()
{
  static uint8_t apiCallResult;

  if (requestedMove) {  

    // Copy requested move to the temp buffer it is not already one and the same
    if (requestedMove != tempBuffer)  
      strcpy(tempBuffer, requestedMove);

    requestedMove=NULL;
  } else {
    strcpy(tempBuffer, "state");
  }

  if ((apiCallResult = apiCall(tempBuffer)) == API_CALL_SUCCESS) {
    updateState(false);
  }

  return apiCallResult;
}
