// ****************************************************************************
//
// LED Controller Class; LED Sequence and Sequence Player Classes
// --------------------------------------------------------------
// Code by W. Witt; V1.00-beta-02; September 2016
//
// This code implements class CwwLedController to control an LED (or
// something similar like a piezo buzzer). It provides an easy interface
// to get an LED to turn on or off, fade up or down, blink or smoothly
// oscillate.
//
// Additionally this file contains the code for the CwwLedSequence
// and CwwLedSequencePlayer classes. These classes, in conjunction with
// CwwLedController, allow LEDs to controlled with predefined action
// sequences.
//
// ****************************************************************************

#include <limits.h>
#include <Arduino.h>

#include <CwwLedController.h>

// ============================================================================
// Private Macros:
// ============================================================================

#define LEVEL_FP_BITS        8  // fixed point bits for LED level values

#define LEVEL_VALUE_ABS_MIN  (   0 << LEVEL_FP_BITS   )
#define LEVEL_VALUE_ABS_MAX  ( 255 << LEVEL_FP_BITS   )
#define LEVEL_VALUE_ABS_MID  ( 255 << LEVEL_FP_BITS-1 )

// ****************************************************************************
// Core LED Controller Class
// ****************************************************************************

// ============================================================================
// Constructors, Destructor
// ============================================================================

CwwLedController::CwwLedController (
  uint8_t       ledPin,
  boolean       usePwm,
  boolean       invertSignal,
  unsigned long blinkPeriod,
  unsigned long oscillatePeriod,
  uint16_t      refreshInterval 
) {

  this->ledPin       = ledPin;
  this->usePwm       = usePwm;
  this->invertSignal = invertSignal;

  this->levelMin = LEVEL_VALUE_ABS_MIN;
  this->levelMax = LEVEL_VALUE_ABS_MAX;

  this->refreshInterval = refreshInterval == 0 ? 1 : refreshInterval;
  setBlinkPeriod     ( blinkPeriod     );
  setOscillatePeriod ( oscillatePeriod );
  this->remainingPhases = 0;

  this->sequencePlayerPtr = NULL;

  pinMode ( ledPin, OUTPUT );
  setMode ( LED_OFF, 0, 0, true );
  drivePin ();

}

// ----------------------------------------------------------------------------

CwwLedController::~CwwLedController () {

}

// ============================================================================
// Public Functions
// ============================================================================

void CwwLedController::turnOff () {

  setMode ( LED_OFF );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::turnOn () {

  setMode ( LED_ON );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::blink ( uint16_t phaseCount ) {

  setMode ( LED_BLINK, phaseCount );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::blinkMax ( uint16_t phaseCount ) {

  setMode ( LED_BLINK_MAX, phaseCount );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::blinkLevel ( uint16_t phaseCount ) {

  setMode ( LED_BLINK_LEVEL, phaseCount );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::turnLow () {

  setMode ( LED_LOW );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::turnHigh () {

  setMode ( LED_HIGH );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::toggle () {

  setMode ( LED_TOGGLE );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::toggleMax () {

  setMode ( LED_TOGGLE_MAX );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::toggleLevel () {

  setMode ( LED_TOGGLE_LEVEL );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::stepDown () {

  decrementLevel ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::stepDown ( uint8_t stepAmount ) {

  decrementLevel ( (uint16_t) stepAmount << LEVEL_FP_BITS );
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::stepUp () {

  incrementLevel ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::stepUp ( uint8_t stepAmount ) {

  incrementLevel ( (uint16_t) stepAmount << LEVEL_FP_BITS );
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::fadeDown () {

  setMode ( LED_FADE_DOWN );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::fadeUp () {

  setMode ( LED_FADE_UP );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::oscillate ( uint16_t phaseCount ) {

  setMode ( LED_OSCILLATE, phaseCount );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::hold () {

  setMode ( LED_HOLD_LEVEL );

}

// ----------------------------------------------------------------------------

boolean CwwLedController::isOn () {

  return ledLevel > 0;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isLow () {

  return ledLevel == levelMin;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isHigh () {

  return ledLevel == levelMax;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isFalling () {

  return updateInterval > 0 && ! ledDirIsUp;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isRising () {

  return updateInterval > 0 &&   ledDirIsUp;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isSteady () {

  return updateInterval == 0;

}

// ============================================================================

void CwwLedController::installSequence ( CwwLedSequence * sequencePtr ) {

  if ( sequencePlayerPtr == NULL ) sequencePlayerPtr = new CwwLedSequencePlayer;

  sequencePlayerPtr->attachSequence ( sequencePtr );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::removeSequence () {

  if ( sequencePlayerPtr != NULL ) delete sequencePlayerPtr;
  sequencePlayerPtr = NULL;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::setSequenceRepeatCount ( uint8_t repeatCount ) {

  if ( sequencePlayerPtr != NULL ) sequencePlayerPtr->setRepeatCount ( repeatCount );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::startSequence () {

  if ( sequencePlayerPtr != NULL ) sequencePlayerPtr->startFirstStep ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::stopSequence () {

  if ( sequencePlayerPtr != NULL ) sequencePlayerPtr->stop ();

}

// ----------------------------------------------------------------------------

uint8_t CwwLedController::valueOfSequenceRepeatCount () {

  return sequencePlayerPtr != NULL && sequencePlayerPtr->valueOfRepeatCount ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isPlayingSequence () {

  return sequencePlayerPtr != NULL && sequencePlayerPtr->isRunning ();

}

// ============================================================================

boolean CwwLedController::setLevel ( uint8_t ledLevelNew ) {

  uint16_t levelNew;
  boolean  success;

  levelNew   = ledLevelNew;
  levelNew <<= LEVEL_FP_BITS;
  success = true;

  if      ( levelNew == LEVEL_VALUE_ABS_MIN ) setMode ( LED_OFF  );
  else if ( levelNew == LEVEL_VALUE_ABS_MAX ) setMode ( LED_ON   );
  else if ( levelNew == levelMin            ) setMode ( LED_LOW  );
  else if ( levelNew == levelMax            ) setMode ( LED_HIGH );
  else if ( usePwm ) {
    success = levelNew >= levelMin && levelNew <= levelMax;
    if      ( levelNew < levelMin ) levelNew = levelMin;
    else if ( levelNew > levelMax ) levelNew = levelMax;
    ledLevel = levelNew;
    ledModeSetting = LED_HOLD_LEVEL;
    ledModeActive  = LED_HOLD_LEVEL;
    drivePin ();
  }
  else {
    success = false;
  }

  return success;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CwwLedController::currentLevel () {

  return ledLevel;

}

// ============================================================================


void CwwLedController::setMode ( cwwEnumLedMode ledModeNew, uint16_t phaseCount, uint8_t stepAmount ) {

  stopSequence ();
  setMode ( ledModeNew, phaseCount, stepAmount, false );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

cwwEnumLedMode CwwLedController::currentMode () {

  return ledModeActive;

}

// ============================================================================

boolean CwwLedController::setBlinkPeriod ( unsigned long newPeriod ) {

  boolean setIsClean;

  setIsClean = newPeriod >= 2;
  blinkPeriod = setIsClean ? newPeriod : 2;
 
  return setIsClean;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::setOscillatePeriod ( unsigned long newPeriod ) {

  boolean setIsClean;

  setIsClean = newPeriod >= 2;
  oscillatePeriod = setIsClean ? newPeriod : 2;
  calcLevelStep ();

  return setIsClean;
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned long CwwLedController::valueOfBlinkPeriod () {

  return blinkPeriod;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned long CwwLedController::valueOfOscillatePeriod () {

  return oscillatePeriod;

}

// ============================================================================

boolean CwwLedController::setRefreshInterval ( uint16_t newInterval ) {

  boolean setIsClean;

  setIsClean = newInterval > 0;
  refreshInterval = setIsClean ? newInterval : 1;
  calcLevelStep ();

  return setIsClean;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint16_t CwwLedController::valueOfRefreshInterval () {

  return refreshInterval;

}

// ============================================================================

boolean CwwLedController::updateIsDue () {

  unsigned long currentTime;
  unsigned long elapsedTime;

  if ( updateInterval > 0 ) {

    currentTime = millis ();

    if ( currentTime < lastDriveTime ) {
      elapsedTime = ( ULONG_MAX - lastDriveTime ) + currentTime;
    }
    else {
      elapsedTime = currentTime - lastDriveTime;
    }

    return elapsedTime >= updateInterval;

  }
  else {

    if ( sequencePlayerPtr == NULL ) return false;
    else                             return sequencePlayerPtr->stepDelayIsDone ();
  }


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::updateNow () {

  if ( sequencePlayerPtr != NULL && sequencePlayerPtr->stepDelayIsDone() ) {

    setMode ( sequencePlayerPtr->modeOfStep(), 0, levelStep, false );
    sequencePlayerPtr->advanceOneStep ();

  }
  else {

    if ( updateIsDue() ) {
      computeState ( ledModeActive );
      drivePin ();
      return true;
    }
    else {
      return false;
    }

  }

}

// ============================================================================

boolean CwwLedController::setLevelMin ( uint8_t levelMinNew ) {

  uint16_t levelRange;
  uint16_t levelOffset;
  uint16_t levelPercent;
  uint16_t levelMinSpec;
  boolean  driveAfterSet;
  boolean  setIsClean;

  if ( levelMinNew == levelMin ) return true;

  driveAfterSet = ledLevel >= levelMin && ledLevel <= levelMax;

  if ( driveAfterSet ) {
    levelRange  = levelMax - levelMin;
    levelOffset = ledLevel - levelMin;
    levelPercent = ( (uint32_t) levelOffset << 15 ) / levelRange;
    // levelPercent: fixed point with 15 fraction bits
  }

  levelMinSpec = levelMinNew << LEVEL_FP_BITS;
  setIsClean = levelMinSpec < levelMax;

  if ( setIsClean ) levelMin = levelMinSpec;
  else              levelMin = levelMax - ( 1 << LEVEL_FP_BITS );

  calcLevelMid ();

  if ( driveAfterSet ) {
    levelRange = levelMax - levelMin;
    levelOffset = ( (uint32_t) levelRange * levelPercent ) >> 15;
    ledLevel = levelMin + levelOffset;
    drivePin ( false );
  }

  calcLevelStep ();

  return setIsClean;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::setLevelMax ( uint8_t levelMaxNew ) {

  uint16_t levelRange;
  uint16_t levelOffset;
  uint16_t levelPercent;
  uint16_t levelMaxSpec;
  boolean  driveAfterSet;
  boolean  setIsClean;

  if ( levelMaxNew == levelMax ) return true;

  driveAfterSet = ledLevel >= levelMin && ledLevel <= levelMax;

  if ( driveAfterSet ) {
    levelRange  = levelMax - levelMin;
    levelOffset = ledLevel - levelMin;
    levelPercent = ( (uint32_t) levelOffset << 15 ) / levelRange;
    // levelPercent: fixed point with 15 fraction bits
  }

  levelMaxSpec = levelMaxNew << LEVEL_FP_BITS;
  setIsClean = levelMaxSpec > levelMin;

  if ( setIsClean ) levelMax = levelMaxSpec;
  else              levelMax = levelMin + ( 1 << LEVEL_FP_BITS );

  calcLevelMid ();

  if ( driveAfterSet ) {
    levelRange = levelMax - levelMin;
    levelOffset = ( (uint32_t) levelRange * levelPercent ) >> 15;
    ledLevel = levelMin + levelOffset;
    drivePin ( false );
  }

  calcLevelStep ();
  
  return setIsClean;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::setLevelRange ( uint8_t levelMinNew, uint8_t levelMaxNew ) {

  uint16_t levelRange;
  uint16_t levelOffset;
  uint16_t levelPercent;
  boolean  driveAfterSet;
  boolean  setIsClean;

  driveAfterSet = ledLevel >= levelMin && ledLevel <= levelMax;

  if ( driveAfterSet ) {
    levelRange  = levelMax - levelMin;
    levelOffset = ledLevel - levelMin;
    levelPercent = ( (uint32_t) levelOffset << 15 ) / levelRange;
    // levelPercent: fixed point with 15 fraction bits
  }

  setIsClean = levelMinNew < levelMaxNew;

  if ( setIsClean ) {
    levelMin = levelMinNew;
    levelMax = levelMaxNew;
  }
  else {
    if ( levelMinNew > levelMaxNew ) {
      levelMin = levelMaxNew;
      levelMax = levelMinNew;
    }
    else {
      if ( levelMinNew == 0 ) {
        levelMin = 0;
        levelMax = 1;
      }
      else {
        levelMin = levelMaxNew - 1;
        levelMax = levelMaxNew;
      }
    }
  }

  levelMin <<= LEVEL_FP_BITS;
  levelMax <<= LEVEL_FP_BITS;
  calcLevelMid ();

  if ( driveAfterSet ) {
    levelRange = levelMax - levelMin;
    levelOffset = ( (uint32_t) levelRange * levelPercent ) >> 15;
    ledLevel = levelMin + levelOffset;
    drivePin ( false );
  }

  return setIsClean;

}

// ----------------------------------------------------------------------------

uint8_t CwwLedController::valueOfLevelMin () {

  return levelMin;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CwwLedController::valueOfLevelMax () {

  return levelMax;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CwwLedController::valueOfLevelStep () {

  return levelStep;

}

// ============================================================================

void CwwLedController::setPwm ( boolean usePwmNew ) {

  usePwm = usePwmNew;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isPwm () {

  return usePwm;

}

// ----------------------------------------------------------------------------

void CwwLedController::setInvert ( boolean invertNew ) {

  invertSignal = invertNew;
  drivePin ( false );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::isInverted () {

  return invertSignal;

}

// ============================================================================
// Private Functions
// ============================================================================

void CwwLedController::setMode (
  cwwEnumLedMode ledModeNew,
  uint16_t       phaseCount,
  uint16_t       stepAmount,
  boolean        forceSet
) {

  cwwEnumLedMode ledModeSpec;

  ledModeSpec = adjustMode ( ledModeNew );
  if ( ledModeSpec != ledModeSetting || forceSet ) {
    computeState ( ledModeSpec, phaseCount, stepAmount );
    drivePin ();
  }

}

// ----------------------------------------------------------------------------

cwwEnumLedMode CwwLedController::adjustMode (
  cwwEnumLedMode ledModeNew
) {

  cwwEnumLedMode ledModeAdjusted;

  // By default, just pass the incoming mode unless one of the following
  // overrides or adjustments is required...
  ledModeAdjusted = ledModeNew;

  // If PWM is not enabled, translate PWM-specific modes to
  // nearest pure digital modes...
  if ( ! usePwm ) {
    switch ( ledModeNew ) {
      case LED_HIGH:
      case LED_STEP_UP:
      case LED_FADE_UP:
        ledModeAdjusted = LED_ON;
        break;
      case LED_LOW:
      case LED_STEP_DOWN:
      case LED_FADE_DOWN:
        ledModeAdjusted = LED_OFF;
        break;
      case LED_FADE_REVERSE:
        ledModeAdjusted = LED_TOGGLE_MAX;
        break;
      case LED_BLINK_LEVEL:
      case LED_OSCILLATE:
        ledModeAdjusted = LED_BLINK_MAX;
        break;
      case LED_HOLD_LEVEL:
        ledModeAdjusted = ledModeActive;
        break;
    }
  }

  // Translate generic toggle model to pure digital or
  // PWM mode depending on historical state...
  if ( ledModeNew == LED_TOGGLE ) {
    switch ( ledModeActive ) {
      case LED_OFF:
      case LED_ON:
      case LED_BLINK_MAX:
        ledModeAdjusted = LED_TOGGLE_MAX;
        break;
      default:
        ledModeAdjusted = LED_TOGGLE_LEVEL;
        break;
    }
  }

  // Translate generic blink model to pure digital or
  // PWM mode depending on historical state...
  if ( ledModeNew == LED_BLINK ) {
    switch ( ledModeActive ) {
      case LED_BLINK_MAX:
      case LED_BLINK_LEVEL:
        ledModeAdjusted = ledModeActive;
        break;
      case LED_OFF:
      case LED_ON:
        ledModeAdjusted = LED_BLINK_MAX;
        break;
      default:
        ledModeAdjusted = LED_BLINK_LEVEL;
        break;
    }
  }

  return ledModeAdjusted;

}

// ----------------------------------------------------------------------------

void CwwLedController::computeState (
  cwwEnumLedMode ledModeNew,
  uint16_t       phaseCount,
  uint16_t       stepAmount
) {

  boolean ledDirIsUpLast;

  if ( stepAmount == 0 ) stepAmount = levelStep;
  ledModeSetting = ledModeNew;
  ledDirIsUpLast = ledDirIsUp;

  switch ( ledModeNew ) {
 
    case LED_OFF:
      ledDirIsUp = false;
      ledLevel = LEVEL_VALUE_ABS_MIN;
      ledModeActive = LED_OFF;
      updateInterval = 0;
 
    case LED_LOW:
      ledDirIsUp = false;
      ledLevel = levelMin;
      ledModeActive = LED_LOW;
      updateInterval = 0;
      break;
 
    case LED_ON:
      ledDirIsUp = true;
      ledLevel = LEVEL_VALUE_ABS_MAX;
      ledModeActive = LED_ON;
      updateInterval = 0;
      break;
 
    case LED_HIGH:
      ledDirIsUp = true;
      ledLevel = levelMax;
      ledModeActive = LED_HIGH;
      updateInterval = 0;
      break;
 
    case LED_TOGGLE_MAX:
      ledDirIsUp = ! levelIsNearMax (); 
      if ( ledDirIsUp ) { ledLevel = LEVEL_VALUE_ABS_MAX; ledModeActive = LED_ON;  }
      else              { ledLevel = LEVEL_VALUE_ABS_MIN; ledModeActive = LED_OFF; }
      updateInterval = 0;
      break;
 
    case LED_TOGGLE_LEVEL:
      ledDirIsUp = ! levelIsNearAbsMax ();
      if ( ledDirIsUp ) { ledLevel = levelMax; ledModeActive = LED_HIGH; }
      else              { ledLevel = levelMin; ledModeActive = LED_LOW;  }
      updateInterval = 0;
      break;
 
    case LED_BLINK_MAX:
      ledDirIsUp = ! levelIsNearMax ();
      ledLevel = ledDirIsUp ? LEVEL_VALUE_ABS_MAX : LEVEL_VALUE_ABS_MIN;
      if ( phaseCount > 0 ) remainingPhases = phaseCount;
      if ( remainingPhases > 0 ) {
        remainingPhases--;
        if ( remainingPhases > 0 ) {
          ledModeActive = LED_BLINK_MAX;
          updateInterval = blinkPeriod / 2;
        }
        else {
          ledModeActive = ledDirIsUp ? LED_ON : LED_OFF;
          updateInterval = 0;
        }
      }
      else {
        ledModeActive = LED_BLINK_MAX;
        updateInterval = blinkPeriod / 2;
      }
      break;
 
    case LED_BLINK_LEVEL:
      ledDirIsUp = ! levelIsNearAbsMax ();
      ledLevel = ledDirIsUp ? levelMax : levelMin;
      if ( phaseCount > 0 ) remainingPhases = phaseCount;
      if ( remainingPhases > 0 ) {
        remainingPhases--;
        if ( remainingPhases > 0 ) {
          ledModeActive = LED_BLINK_LEVEL;
          updateInterval = blinkPeriod / 2;
        }
        else {
          ledModeActive = ledDirIsUp ? LED_HIGH : LED_LOW;
          updateInterval = 0;
        }
      }
      else {
        ledModeActive = LED_BLINK_LEVEL;
        updateInterval = blinkPeriod / 2;
      }
      break;
 
    case LED_STEP_DOWN:
      ledDirIsUp = false;
      decrementLevel ( stepAmount );
      if ( ledLevel == levelMin ) ledModeActive = LED_LOW;
      else                        ledModeActive = LED_HOLD_LEVEL;
      updateInterval = 0;
      break;
 
    case LED_STEP_UP:
      ledDirIsUp = true;
      incrementLevel ( stepAmount );
      if ( ledLevel == levelMax ) ledModeActive = LED_HIGH;
      else                        ledModeActive = LED_HOLD_LEVEL;
      updateInterval = 0;
      break;
 
    case LED_FADE_DOWN: 
    case LED_FADE_UP:
    case LED_FADE_REVERSE:
      switch ( ledModeNew ) {
        case LED_FADE_DOWN:
          ledDirIsUp = false;
          break;
        case LED_FADE_UP:
          ledDirIsUp = true;
          break;
        case LED_FADE_REVERSE:
          ledDirIsUp = ! ledDirIsUp;
          break;
      }
      if ( ledDirIsUp ) {
        incrementLevel ();
        if ( ledLevel == levelMax ) {
          ledModeActive = LED_HIGH;
          updateInterval = 0;
        }
        else {
          ledModeActive = LED_FADE_UP;
          updateInterval = refreshInterval;
        }
      }
      else {
        decrementLevel ();
        if ( ledLevel == levelMin ) {
          ledModeActive = LED_LOW;
          updateInterval = 0;
        }
        else {
          ledModeActive = LED_FADE_DOWN;
          updateInterval = refreshInterval;
        }
      }
      break;
 
    case LED_OSCILLATE:
      if ( ledLevel == levelMin || ledLevel == levelMax ) {
        ledDirIsUp = ! ledDirIsUp;
      }
      if ( ledDirIsUp ) incrementLevel ();
      else              decrementLevel ();
      if ( phaseCount > 0 ) remainingPhases = phaseCount;
      if ( remainingPhases > 0 && ( ledLevel == levelMin || ledLevel == levelMax ) ) {
        remainingPhases--;
        if ( remainingPhases > 0 ) {
          ledModeActive = LED_OSCILLATE;
          updateInterval = refreshInterval;
        }
        else {
          ledModeActive = ledDirIsUp ? LED_HIGH : LED_LOW;
          updateInterval = 0;
        }
      }
      else {
        ledModeActive = LED_OSCILLATE;
        updateInterval = refreshInterval;
      }
      break;

    case LED_HOLD_LEVEL:
      ledModeActive = LED_HOLD_LEVEL;
      updateInterval = 0;
      break;

  }  // switch ( ledModeNew )

}

// ============================================================================

boolean CwwLedController::calcLevelStep () {

  uint16_t      levelRange;
  unsigned long oscillatePhase;
  unsigned long stepsPerOscillatePhase;
  boolean       setIsClean;

  levelRange = levelMax - levelMin; 
  oscillatePhase = oscillatePeriod / 2;

  stepsPerOscillatePhase = oscillatePhase / refreshInterval;
  if ( oscillatePhase % refreshInterval > 0 ) stepsPerOscillatePhase++;
  setIsClean = stepsPerOscillatePhase > 0;
  if ( ! setIsClean ) stepsPerOscillatePhase = 1;

  levelStep = levelRange / stepsPerOscillatePhase;
  setIsClean = setIsClean && levelStep > 0;
  if ( levelStep == 0 ) levelStep = 1;

  return setIsClean;

}

// ----------------------------------------------------------------------------

void CwwLedController::decrementLevel () {

  decrementLevel ( levelStep );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::decrementLevel ( uint16_t delta ) {

  int32_t ledLevelNew;

  ledLevelNew = (int32_t) ledLevel - delta;
  if ( ledLevelNew < levelMin ) ledLevel = levelMin;
  else                          ledLevel = ledLevelNew;

}

// ----------------------------------------------------------------------------

void CwwLedController::incrementLevel () {

  incrementLevel ( levelStep );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedController::incrementLevel ( uint16_t delta ) {

  int32_t ledLevelNew;

  ledLevelNew = (int32_t) ledLevel + delta;
  if ( ledLevelNew > levelMax ) ledLevel = levelMax;
  else                          ledLevel = ledLevelNew;

}

// ============================================================================

void CwwLedController::calcLevelMid () {

  levelMid = levelMin + ( levelMax - levelMin ) / 2;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::levelIsNearMax () {

  return ledLevel > LEVEL_VALUE_ABS_MID;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedController::levelIsNearAbsMax () {

  return ledLevel > levelMid;

}

// ============================================================================

void CwwLedController::drivePin ( boolean markDriveTime ) {

  uint8_t ledLevelEff;

  ledLevelEff = ( invertSignal ? LEVEL_VALUE_ABS_MAX - ledLevel : ledLevel ) >> LEVEL_FP_BITS;

  if      ( ledLevelEff == 0 ) digitalWrite ( ledPin, LOW         );
  else if ( usePwm )           analogWrite  ( ledPin, ledLevelEff );
  else                         digitalWrite ( ledPin, HIGH        );

  if ( markDriveTime ) lastDriveTime = millis ();

}

// ****************************************************************************
// LED Action Sequence Class (defines action/event sequence)
// ****************************************************************************

// ============================================================================
// Constructors, Destructor
// ============================================================================

CwwLedSequence::CwwLedSequence () {

   startOfSequencePtr = NULL;
   repeatCount        = 1;
   attachCount        = 0;
   
}

// ----------------------------------------------------------------------------

CwwLedSequence::~CwwLedSequence () {

  discardAll ();

}

// ============================================================================
// Public Functions
// ============================================================================

void CwwLedSequence::addStep ( unsigned long timeToStepMs, cwwEnumLedMode modeOfStep ) {

  structSequenceStep * newStepPtr;
  structSequenceStep * lastStepPtr;

  newStepPtr = new structSequenceStep;
  newStepPtr->timeToStepMs = timeToStepMs;
  newStepPtr->modeOfStep   = modeOfStep;
  newStepPtr->nextStepPtr  = NULL;

  if ( startOfSequencePtr == NULL ) {
    startOfSequencePtr = newStepPtr;
  }
  else {
    lastStepPtr = startOfSequencePtr;
    while ( lastStepPtr->nextStepPtr != NULL ) lastStepPtr = lastStepPtr->nextStepPtr;
    lastStepPtr->nextStepPtr = newStepPtr;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedSequence::discardAll ( boolean forceDiscard ) {

  structSequenceStep * delStepPtr;
  structSequenceStep * nextStepPtr;

  if ( attachCount == 0 || forceDiscard ) {

    delStepPtr = startOfSequencePtr;
    while ( delStepPtr != NULL ) {
      nextStepPtr = delStepPtr->nextStepPtr;
      delete delStepPtr;
      delStepPtr = nextStepPtr;
    }

    startOfSequencePtr = NULL;
    repeatCount = 1;

  }

}

// ----------------------------------------------------------------------------

void CwwLedSequence::setRepeatCount ( uint8_t repeatCount ) {

  this->repeatCount = repeatCount;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CwwLedSequence::valueOfRepeatCount () {

  return repeatCount;

}

// ----------------------------------------------------------------------------

uint8_t CwwLedSequence::valueOfAttachCount () {

  return attachCount;

}

// ============================================================================
// Private Functions
// ============================================================================

void  CwwLedSequence::attachPlayer () {

  attachCount++;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void  CwwLedSequence::detachPlayer () {

  if ( attachCount > 0 ) attachCount--;

}

// ****************************************************************************
// LED Action Sequence Player Class (manages advancing through sequence)
// ****************************************************************************

// ============================================================================
// Constructors, Destructor
// ============================================================================

CwwLedSequencePlayer::CwwLedSequencePlayer () {

  attachedSequencePtr = NULL;
  currentStepPtr      = NULL;

  iterationsToPlay = 1;
  currentIteration = 0;

}

// ----------------------------------------------------------------------------

CwwLedSequencePlayer::~CwwLedSequencePlayer () {

  detachSequence ();

}

// ============================================================================
// Private Functions (for use by CwwLedController friend)
// ============================================================================

void CwwLedSequencePlayer::attachSequence ( CwwLedSequence * sequencePtr ) {

  if ( attachedSequencePtr != NULL ) detachSequence ();

  attachedSequencePtr = sequencePtr;
  attachedSequencePtr->attachPlayer ();
  currentStepPtr = attachedSequencePtr->startOfSequencePtr;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedSequencePlayer::detachSequence () {

  if ( attachedSequencePtr != NULL ) attachedSequencePtr->detachPlayer ();
  
  attachedSequencePtr = NULL;

}

// ----------------------------------------------------------------------------

void CwwLedSequencePlayer::setRepeatCount ( uint8_t repeatCount ) {

  iterationsToPlay = repeatCount;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

uint8_t CwwLedSequencePlayer::valueOfRepeatCount () {

  return iterationsToPlay;

}

// ----------------------------------------------------------------------------


boolean CwwLedSequencePlayer::startFirstStep () {

  if ( attachedSequencePtr->startOfSequencePtr != NULL ) {
    currentStepPtr = attachedSequencePtr->startOfSequencePtr;
    currentIteration = 1;
    delayToStepTimer.start( currentStepPtr->timeToStepMs );
    return true;
  }
  else {
    return false;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedSequencePlayer::advanceOneStep () {

  uint16_t effectiveIterations;
  boolean  stepSuccess;

  if ( attachedSequencePtr->startOfSequencePtr == NULL ) return false;

  if ( currentStepPtr->nextStepPtr != NULL ) {
    currentStepPtr = currentStepPtr->nextStepPtr;
    stepSuccess = true;
  }
  else {
    effectiveIterations = iterationsToPlay * attachedSequencePtr->repeatCount;
    stepSuccess = effectiveIterations == 0 || currentIteration < effectiveIterations;
    if ( stepSuccess ) {
      currentStepPtr = attachedSequencePtr->startOfSequencePtr;
      currentIteration++;
    }
    else {
      stop ();
    };
  }

  if ( stepSuccess ) delayToStepTimer.start( currentStepPtr->timeToStepMs );

  return stepSuccess;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CwwLedSequencePlayer::stop () {
  
  delayToStepTimer.stop ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedSequencePlayer::resume () {
  
  if ( isPaused() ) {
    delayToStepTimer.resume ();
    return true;
  }
  else {
    return true;
  }

}

// ----------------------------------------------------------------------------

boolean CwwLedSequencePlayer::isRunning () {

  return delayToStepTimer.isRunning ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedSequencePlayer::isPaused () {

  return delayToStepTimer.isPaused ();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedSequencePlayer::stepDelayIsDone () {

  boolean delayIsDone;

  delayIsDone = delayToStepTimer.hasElapsed() && delayToStepTimer.isRunning();

  if ( delayIsDone && atEndOfSequence() ) delayToStepTimer.stop ();

  return delayIsDone;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

boolean CwwLedSequencePlayer::atEndOfSequence () {

  return currentStepPtr->nextStepPtr == NULL;

}

// ----------------------------------------------------------------------------

cwwEnumLedMode CwwLedSequencePlayer::modeOfStep () {

  return currentStepPtr->modeOfStep;

}

// ****************************************************************************
