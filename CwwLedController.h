// *****************************************************************************
//
// LED Controller Class
// --------------------
// Code by W. Witt; V1.00-beta-01; July 2016
//
// This code defines a class to control an LED (or something similar
// like a piezo buzzer). It provides an easy interface to get an LED to
// to turn on or off, fade up or down, blink or smoothly oscillate while
// abstracting all of the code that would otherwise be required to track
// turn-on or turn-off times, brightness levels or fade steps.
//
// Depending on the microcontroller pin assigned to a particular LED
// controller object, whether the pin is PWM capable, smooth fade and
// oscillation functons may not be available (potentially resulting in
// hard on/off or blink behavior instead).
//
// If multiple LED controllers need to be operated in parallel, and all
// must maintain synchronization, special measures may be required.
// Synchronization may not be automatic especically if LEDs are set to
// oscillate and different LEDs have different min/max settings. If
// synchronization is a problem, a set of LED controller instances may
// be connected via a handshake mechanism.
// 
// *****************************************************************************

#ifndef CwwLedController_h
#define CwwLedController_h

// *****************************************************************************

#include <Arduino.h>

// =============================================================================

#define CWW_LC_SYNC_WORD uint8_t

// ----------------------------------------------------------------------------

class CwwLedController {

  public:

    // Public Types:

    enum enumLedMode {
      LED_OFF,           // Turn LED complete off
      LED_ON,            // Turn LED fully on
      LED_LOW,           // Turn LED to minimum level (as defined by setLevelMin); requires PWM
      LED_HIGH,          // Turn LED to maximum level (as defined by setLevelMax); requires PWN
      LED_TOGGLE,        // Toggle the state of the LED (off/on or low/high, depending on context)
      LED_TOGGLE_MAX,    // Toggle the state of the LED, leaving it full on or off 
      LED_TOGGLE_LEVEL,  // Toggle the state of the LED, leaving it low or high; requires PWM
      LED_BLINK,         // Start blinking the LED (off/on or low/high, dependong on context) (3)
      LED_BLINK_MAX,     // Start blinking the LED between full off/on states (3)
      LED_BLINK_LEVEL,   // Start blinking the LED between low/high states; requires PWM (3)
      LED_STEP_DOWN,     // Decrement LED brightness level by one step (PWM) (1)
      LED_STEP_UP,       // Increment LED brightness level by one step (PWM) (1)
      LED_FADE_DOWN,     // Fade the LED down until it reaches LED_LOW (PWM) (2) (3)
      LED_FADE_UP,       // Fade the LED up until it reaches LED_HIGH (PWM) (2) (3)
      LED_FADE_REVERSE,  // Reverse the direction of the last fade (PWM) (3)
      LED_OSCILLATE,     // Oscillate the LED, repeatedly fading up, down, up, etc. (PWM) (3)
      LED_HOLD_LEVEL     // Stop the LED at the current level
    };
    // 1: Default step is derived from a) distance between minimum and
    //    maxium brightness levels, b) oscillation period and c) refresh
    //    interval. See valueOfLevelStep().
    // 2: Fade speed depends on oscillation period. One full min to max
    //    or max to min fade has a duration of one oscillation phase
    //    (i.e half a period).
    // 3: Requires occasional calls to updateNow(). Ideally, delay
    //    between calls should not exceed refresh interval, although
    //    for blink mode, one call per phase is sufficient.

    // Public Constants:

    // Public Variables:

    // Public Functions:

             CwwLedController ( uint8_t       ledPin,                   // Pin for LED connection; required argument to constructor
                                boolean       usePwn          = false,  // Set to true if pin is PWM-capable
                                boolean       invertSignal    = false,  // Set to true for active low pin; e.g. if driving cathode of LED (e.g. for some RGB LEDs)
                                unsigned long blinkPeriod     =  1000,  // Blink period in ms (one period is two phases)
                                unsigned long oscillatePeriod =  1000,  // Smooth oscillation period in ms (one period is two phases)
                                uint16_t      refreshInterval =    20   // Interval in ms between updates of LED level (e.g. for fade and oscillate) 
                              );
    virtual ~CwwLedController ();

    void    turnOff   ();                             // Turn LED completely off
    void    turnOn    ();                             // Turn LED fully on
    void    blink     ( uint16_t phaseCount = 0 );    // Start LED blinking for phaseCount on/off phases; default phaseCount of 0 blinks forever (3)
    void    turnLow   ();                             // Set LED to lowest setting (see setLevelMin, valueOfLevelMin)
    void    turnHigh  ();                             // Set LED to highest setting (see setLevelMax, valueOfLevelMax)
    void    toggle    ();                             // Toggle state of LED (on->off, off->on, low->high, high->low)
    void    stepDown  ();                             // Step LED brightness down one default increment (1) 
    void    stepDown  ( uint8_t  stepAmount );        // Step LED brightness down by specified increment
    void    stepUp    ();                             // Step LED brightness up one default increment (1)
    void    stepUp    ( uint8_t  stepAmount );        // Step LED brightness up by specified increment
    void    fadeDown  ();                             // Start LED on a fade towards is lowest setting (2) (3)
    void    fadeUp    ();                             // Start LED on a fade towards is highest setting (2) (3)
    void    oscillate ( uint16_t phaseCount = 0 );    // (3)
    void    hold      ();

    boolean isOn      ();  // true if LED is not off
    boolean isLow     ();  // true if LED is at its lowest level (see setLevelMin)
    boolean isHigh    ();  // true if LED is at its maximum level (see setLevelMax)
    boolean isFalling ();  // true if LED is changing and brightness is decreasing
    boolean isRising  ();  // true if LED is changing and brightness is increasing
    boolean isSteady  ();  // true if LED is in a stable state

    boolean     setLevel ( uint8_t ledLevelNew );  // Force LED level to specified value
    uint8_t     currentLevel ();
    // Level may only be full off (0), full on (255) or in range of
    // minimum level to maximum level. Out of range level will be
    // clamped to min or max.

    void        setMode ( enumLedMode ledModeNew, uint16_t phaseCount = 0, uint8_t stepAmount = 0 );
    enumLedMode currentMode  ();

    boolean       setBlinkPeriod     ( unsigned long newPeriod   );  // period in milliseconds (ms)
    boolean       setOscillatePeriod ( unsigned long newPeriod   );  // period in milliseconds (ms)
    unsigned long valueOfBlinkPeriod     ();
    unsigned long valueOfOscillatePeriod ();

    boolean       setRefreshInterval ( uint16_t      newInterval );  // interval in ms
    uint16_t      valueOfRefreshInterval ();

    boolean       updateIsDue ();  // if true, updateNow() needs to be called
    boolean       updateNow   ();  // prior test of updateIsDue() is not required; will only update if due 

    boolean setLevelMin   ( uint8_t levelMinNew );  // pwm level in range of 0 to 254
    boolean setLevelMax   ( uint8_t levelMaxNew );  // pwm level in range of 1 to 255
    boolean setLevelRange ( uint8_t levelMinNew, uint8_t levelMaxNew );
    uint8_t valueOfLevelMin  ();
    uint8_t valueOfLevelMax  ();
    uint8_t valueOfLevelStep ();  // auto-computed (1)

    void    setPwm ( boolean usePwmNew );
    boolean isPwm  ();

    void    setInvert  ( boolean invertNew );
    boolean isInverted ();

    CWW_LC_SYNC_WORD attachSyncHandshake ( CWW_LC_SYNC_WORD * flagPtr, boolean isFirst = false );
    void             initSyncHandshake ();

  private:

    // Private Constants:

    const CWW_LC_SYNC_WORD syncFlagSyncedBit = 0x1 << (sizeof(CWW_LC_SYNC_WORD)*8-1);

    // Private Variables:

    uint8_t ledPin;
    boolean usePwm;
    boolean invertSignal;

    enumLedMode ledModeActive;
    enumLedMode ledModeSetting;
    uint16_t    ledLevel;
    boolean     ledDirIsUp;

    uint16_t levelMin;
    uint16_t levelMax;
    uint16_t levelMid;
    uint16_t levelStep;

    uint16_t      refreshInterval;

    unsigned long blinkPeriod;
    unsigned long oscillatePeriod;
    uint16_t      remainingPhases;

    unsigned long updateInterval;
    unsigned long lastDriveTime;

    CWW_LC_SYNC_WORD * syncFlagPtr;
    CWW_LC_SYNC_WORD   syncFlagMeBit;
    CWW_LC_SYNC_WORD   syncFlagCheckMask;

    // Private Functions:

    void setMode ( enumLedMode ledModeNew, uint16_t phaseCount, uint16_t stepAmount, boolean forceSet );

    enumLedMode adjustMode   ( enumLedMode ledModeNew );
    void        computeState ( enumLedMode ledModeNew, uint16_t phaseCount = 0, uint16_t stepAmount = 0 );

    boolean calcLevelStep  ();
    void    decrementLevel ();
    void    decrementLevel ( uint16_t delta );
    void    incrementLevel ();
    void    incrementLevel ( uint16_t delta );
 
    void    calcLevelMid   ();
    boolean levelIsNearMax ();
    boolean levelIsNearTop ();

    boolean syncAchieved ();

    void drivePin ( boolean markDriveTime = true );

};

// *****************************************************************************

#endif

// *****************************************************************************
