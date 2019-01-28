/* Copyright 2019 Georg Voigtlaender gvoigtlaender@googlemail.com */
#include "./config.h"
#if WITH_FASTLED == 1
#include <FastLED.h>
#endif
#include <Arduino.h>

extern "C" {
  /*
  __extension__ typedef int __guard __attribute__((mode (__DI__)));
  int __cxa_guard_acquire(__guard g) {
    return !(char )(g);
  }
  void __cxa_guard_release(__guard *g) {
    (char *)g = 1;
  }
  */
  volatile unsigned long timer0_millis = 0;
};

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
    digitalWrite(LATCH_OUT, HIGH);
  }
  virtual bool control() {
    /* code */

    if ( millis() > m_ulMillis + 60000 )
      m_bDone = true;
    delay(1);
    return m_bDone;
  }

  virtual void shutdown()
  {
    digitalWrite(LATCH_OUT, LOW);
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

  virtual void setup() override
  {
    CControlBase::setup();
    m_ulNextSwitch = m_ulMillis-1;
  }

  virtual bool control() override
  {
    if (millis() > m_ulNextSwitch )
    {
      m_bState ^= 1;
      digitalWrite(LED_OUT, m_bState);
      m_ulNextSwitch += m_ulInterval;
    }

    return CControlBase::control();
  }

  virtual void shutdown() override
  {
    CControlBase::shutdown();
  }

protected:
  unsigned long m_ulInterval;
  unsigned long m_ulNextSwitch;
  bool m_bState;
};
//WITH_BLINK

#elif WITH_TRAFFICLIGHT_2 == 1
typedef struct
{
  CRGB up;
  CRGB down;
  unsigned int delay;
}strTL;

#define NUM_LEDS 2
#define LED_TYPE WS2812B
#define BRIGHTNESS  8
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 1

strTL oTL[2] =
{
  { CRGB::Red,  CRGB::Black,  5000},
  { CRGB::Black,  CRGB::Green,  5000},
};
CRGB leds[NUM_LEDS];
class CControlTL2 : public CControlBase
{
public:
  CControlTL2() : CControlBase()
  ,m_nState(0)
  {

  }

  virtual void setup() override
  {
    CControlBase::setup();
    m_ulNextSwitch = m_ulMillis;
    delay(200);
    FastLED.addLeds<LED_TYPE, LED_OUT,
         COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
  }

  virtual bool control() override
  {
    if ( millis() > m_ulNextSwitch )
    {
        leds[0] = oTL[m_nState].up;
        leds[1] = oTL[m_nState].down;
        FastLED.show();
        // FastLED.delay(1000 / UPDATES_PER_SECOND);
        m_ulNextSwitch += oTL[m_nState].delay;
        m_nState = (m_nState==0)?1:0;
    }

    return CControlBase::control();
  }

  virtual void shutdown() override
  {
    for ( unsigned int n=0; n<NUM_LEDS; n++ )
     leds[n] = CRGB::Black;
    FastLED.show();
    FastLED.clear(true);
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
  //pinMode(LED_OUT, OUTPUT);
  delay(100);  // power-up safety delay

  pObj = new CControlTL2();
  pObj->setup();

}

void loop() {

  if ( !pObj->control() )
    return;

  pObj->shutdown();

  digitalWrite(LATCH_OUT, LOW);
  delay(100000);
}
