#include <stdlib.h>
#include <string.h>
#include "stateclient.h"
#include "misc.h"
#include "fujinet-network.h"

// Internal to this file
static char url[160];
char *requestedMove;

/*
 * @brief Makes an Api call, returning true if valid payload received
 * Returns API_CALL_*:
 *  1 - successfully received a payload
 *  2 - async - received payload, still in process, call for more data
 *  0 - error - aborted
*/
uint8_t apiCall(char *path) {
  static int16_t read;
  static char *query;
 
  strcpy(url, "n:");
  strcat(url, serverEndpoint);
  strcat(url, path);
  query = state.localPlayer[state.currentLocalPlayer].query;
  strcat(url, query );
  strcat(url, query[0] ? "&bin=1" : "?bin=1");
  
  if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE)) {
    return API_CALL_ERROR;
  } 
  
  read = network_read(url, &clientState.firstByte, sizeof(clientState.game));
  network_close(url);

  // If no bytes read, set first byte of clientState to 0, which is the number of tables or players
  if (read<=0) { 
    clientState.firstByte=0;
    return API_CALL_ERROR;
  }

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
    
    // Map local players
    state.currentLocalPlayer=state.localPlayerIsActive=0;

    for(j=0;j<clientState.game.playerCount;j++) {
      for(i=0;i<prefs.localPlayerCount;i++) {
        if (strcmp(clientState.game.players[j].name, prefs.localPlayer[i].name)==0) {
          state.localPlayer[i].index = j;
          // If this player is the active player, set the appropriate local player index and flag it is a local player's turn
          if (j == clientState.game.activePlayer) {
            state.currentLocalPlayer = i;
            state.localPlayerIsActive = true;
          }
          break;
        }
      }
    }
  }

  return apiCallResult;
}
