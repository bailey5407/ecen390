/* TO-DO: 
    Fix detector. For some reason we're only detecting shots on player 0 and 1. Maybe it's a problem with how we're setting our transmitter??
*/
#include <stdio.h>
#include <stdbool.h>

#include "game.h"
#include "hitLedTimer.h"
#include "interrupts.h"
#include "isr.h"
#include "runningModes.h"
#include "switches.h"
#include "buttons.h"
#include "filter.h"
#include "detector.h"
#include "intervalTimer.h"
#include "lockoutTimer.h"
#include "sound/sound.h"
#include "invincibilityTimer.h"
#include "mio.h"
#include "utils.h"
#include "reloadTimer.h"
#include "trigger.h"
#include "transmitter.h"
#include "histogram.h"

#define SWITCH0 0x1
#define TEAM_A_PLAYERNUMBER 6 
#define TEAM_B_PLAYERNUMBER 9
#define DEFAULT_STARTING_LIVES 3
#define DEFAULT_HITS_PER_LIFE 5
#define DEFAULT_SHOTS_PER_CLIP 10
#define FILTER_COUNT 10

// stuff for the ISR and the game init
#define ISR_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_0 // Used by the ISR.
#define TOTAL_RUNTIME_TIMER INTERVAL_TIMER_TIMER_1 // Used to compute total run-time.
#define MAIN_CUMULATIVE_TIMER INTERVAL_TIMER_TIMER_2 // Used to compute cumulative run-time in main.
#define STARTUP_INVINCIBILITY_SECONDS 1
#define RESPAWN_INVINCIBILITY_SECONDS 5

// defines for reability
#define INTERRUPTS_CURRENTLY_ENABLED true

//keeps track of number of lives
static uint16_t lives;

//keeps track of whether we can shoot or not
static bool ableToShoot = true;

//keeps track of the total hits
static uint16_t hitCount;

// ignored frequencies array
static bool ignoredFrequencies[FILTER_FREQUENCY_COUNT];

// This game supports two teams, Team-A and Team-B.
// Each team operates on its own configurable frequency.
// Each player has a fixed set of lives and once they
// have expended all lives, operation ceases and they are told
// to return to base to await the ultimate end of the game.
// The gun is clip-based and each clip contains a fixed number of shots
// that takes a short time to reload a new clip.
// The clips are automatically loaded.
// Runs until BTN3 is pressed.
void game_twoTeamTag() {

  /**********************initialization***********************************/

  //start the invincibility timer to ignore hits in the bootup
  invincibilityTimer_start(STARTUP_INVINCIBILITY_SECONDS);

  //set the remaining shots
  trigger_setRemainingShotCount(DEFAULT_SHOTS_PER_CLIP);

  //keep track of hits
  uint16_t hitCount = 0;

  //set initial lives
  lives = DEFAULT_STARTING_LIVES;

  //initialize stuff like detector, etc.
  runningModes_initAll();

  // determine transmit and ignored frequencies
  // Init the ignored-frequencies so no frequencies are ignored.
  if(switches_read() & SWITCH0){
    transmitter_setFrequencyNumber(TEAM_B_PLAYERNUMBER);
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
      if(i == TEAM_A_PLAYERNUMBER) ignoredFrequencies[i] = false;
      else ignoredFrequencies[i] = true;
    }
  }
  //"TEAM A" case
  else{
    transmitter_setFrequencyNumber(TEAM_A_PLAYERNUMBER);
    for(uint8_t i = 0; i < FILTER_COUNT; i++){
      if(i == TEAM_B_PLAYERNUMBER) ignoredFrequencies[i] = false;
      else ignoredFrequencies[i] = true;
    }
  }
  detector_setIgnoredFrequencies(ignoredFrequencies);
  
  // set volume to max
  sound_setVolume(SOUND_VOLUME_2);

  //Finally, play start up sound
  sound_playSound(sound_gameStart_e);

  //enabling the trigger and the associated interrupts
  trigger_enable(); // Makes the state machine responsive to the trigger.
  interrupts_enableTimerGlobalInts(); // Allow timer interrupts.
  interrupts_startArmPrivateTimer();  // Start the private ARM timer running.
  intervalTimer_reset(ISR_CUMULATIVE_TIMER); // Used to measure ISR execution time.
  intervalTimer_reset(TOTAL_RUNTIME_TIMER); // Used to measure total program execution time.
  intervalTimer_reset(MAIN_CUMULATIVE_TIMER); // Used to measure main-loop execution time.
  intervalTimer_start(TOTAL_RUNTIME_TIMER);   // Start measuring total execution time.
  interrupts_enableArmInts(); // ARM will now see interrupts after this.
  lockoutTimer_start(); // Ignore erroneous hits at startup (when all power values are essentially 0).

  /**********************game loop***********************************/

  while ((!(buttons_read() & BUTTONS_BTN3_MASK)) && lives > 0) { // Run until you detect BTN3 pressed.
    intervalTimer_start(MAIN_CUMULATIVE_TIMER); // Measure run-time when you are doing something.
    // Run filters, compute power, run hit-detection.
    detector(INTERRUPTS_CURRENTLY_ENABLED); // Interrupts are currently enabled.
    if (detector_hitDetected()) {           // Hit detected
      hitCount++;                           // increment the hit count.
      detector_clearHit();                  // Clear the hit.
      //if there are hits in life remaining, play hit sound, else play death sound and increment corresponding values
      if(hitCount < DEFAULT_HITS_PER_LIFE){
        sound_playSound(sound_hit_e);
      }
      else if (hitCount >= DEFAULT_HITS_PER_LIFE){
        hitCount = 0;
        lives--;
        sound_playSound(sound_loseLife_e);
        invincibilityTimer_start(RESPAWN_INVINCIBILITY_SECONDS);
      }
      detector_hitCount_t hitCounts[FILTER_COUNT]; // Store the hit-counts here.
      detector_getHitCounts(hitCounts);       // Get the current hit counts.
      histogram_plotUserHits(hitCounts);      // Plot the hit counts on the TFT.
    }
    intervalTimer_stop(MAIN_CUMULATIVE_TIMER); // All done with actual processing.
  }
  sound_playSound(sound_gameOver_e);
  //return to base loop
  trigger_disable();
  while (!(buttons_read() & BUTTONS_BTN3_MASK)) {
    utils_msDelay(1000); // one second delay
    if (!(sound_isBusy())) {
      sound_playSound(sound_returnToBase_e);
    }
  }

  /**********************post game loop actions**********************/

  interrupts_disableArmInts(); // Done with loop, disable the interrupts.
  hitLedTimer_turnLedOff();    // Save power :-)
  printf("That's all folks\n");
}
