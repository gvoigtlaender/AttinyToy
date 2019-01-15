/* Copyright 2019 Georg Voigtlaender gvoigtlaender@googlemail.com */
#include <Arduino.h>

#include "./config.h"

#if WITH_FASTLED == 1
#include <FastLED.h>
#endif

// fixed for pcb
#define LATCH_OUT     PB3
#define LED_OUT       PB1

class CControlBase
{
public:
  CControlBase()
  :m_bDone(false)
  ,m_ulMillis(0)
  {

  }
  virtual ~CControlBase()
  {
    ;
  }
  virtual void setup() {
    /* code */
    m_ulMillis = millis();
    digitalWrite(LED_OUT, HIGH);
  }
  virtual bool control() {
    /* code */

    if ( millis() > m_ulMillis + 10000 )
      m_bDone = true;

    return m_bDone;
  }

  virtual void shutdown()
  {
    digitalWrite(LED_OUT, LOW);
  }

protected:
  bool  m_bDone;
  int64_t m_ulMillis;
};

#if WITH_BLINK == 1
class CControl_Blink : public CControlBase
{
public:
  CControl_Blink(unsigned long ulInterval = 500) : CControlBase()
  ,m_ulInterval(ulInterval)
  ,m_ulNextSwitch(0)
  ,m_bState(false)
  {
    ;
  }

  virtual void setup()
  {
    CControlBase::setup();
    m_ulNextSwitch = m_ulMillis-1;
  }

  virtual bool control()
  {
    if (millis() > m_ulNextSwitch )
    {
      m_bState ^= 1;
      digitalWrite(LED_OUT, m_bState);
      m_ulNextSwitch += m_ulInterval;
    }

    return CControlBase::control();
  }

  virtual void shutdown()
  {
    CControlBase::shutdown();
  }

protected:
  unsigned long m_ulInterval;
  unsigned long m_ulNextSwitch;
  bool m_bState;
};
//WITH_BLINK

#else if WITH_TRAFFICLIGHT_2 == 1
typedef struct
{
  CRGB up;
  CRGB down;
  unsigned int delay;
}strTL;

strTL oTL[2] =
{
  { CRGB::Red,  CRGB::Black,  1000},
  { CRGB::Black,  CRGB::Green,  1000},
};
CRGB leds[2];
class CControlTL2 : public CControlBase
{
public:
  CControlTL2() : CControlBase()
  ,m_nState(0)
  {

  }

  virtual void setup()
  {
    CControlBase::setup();
    m_ulNextSwitch = m_ulMillis-1;
    FastLED.addLeds<WS2812B, LED_OUT, RGB>(leds, 2);
  }

  virtual bool control()
  {
    if ( millis() > m_ulNextSwitch )
    {
        leds[0] = oTL[m_nState].up;
        leds[1] = oTL[m_nState].down;
        FastLED.show();
        m_ulNextSwitch += oTL[m_nState].delay;
        m_nState = (m_nState+1 % 2);
    }

    return CControlBase::control();
  }

protected:
  int m_nState;
  unsigned long m_ulNextSwitch;
};
#endif //WITH_TRAFFICLIGHT_2


// Main Control Code
CControlBase* pObj = NULL;

void setup() {

  pinMode(LATCH_OUT, OUTPUT);
  digitalWrite(LATCH_OUT, HIGH);
  pinMode(LED_OUT, OUTPUT);
  delay(100);  // power-up safety delay

  pObj = new CControlTL2();

}

void loop() {

  if ( !pObj->control() )
    return;

  pObj->shutdown();

  digitalWrite(LATCH_OUT, LOW);
  delay(100000);
}
