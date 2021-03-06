/************************************************************************/
/*                                                                      */
/*    LEDs.cpp                                                          */
/*                                                                      */
/*    LEDs State machine                                                */
/*                                                                      */
/************************************************************************/
/*    Author:     Keith Vogel                                           */
/*    Copyright 2016, Digilent Inc.                                     */
/************************************************************************/
/*  Revision History:                                                   */
/*    6/16/2016(KeithV): Created                                        */
/************************************************************************/
#include    <OpenScope.h>

STATE LEDTask(void)
{   
    static  uint32_t    tBtn            = 0;
    static  bool        fBtn            = false;
    static  uint32_t    tLed            = SYSGetMilliSecond();
    static  uint32_t    ip1             = 0;
    static  uint32_t    ip2             = 0;
    static  uint32_t    ip3             = 0;
    static  IPv4        ipOpenScope     = {0};
    static  uint32_t    i               = 0;
    static  bool        fBlockIOBusL    = false;
    static  bool        fNoLEDs         = false;
            bool        fBlink          = false;
            bool        fStopBlink      =   pjcmd.trigger.state.processing == Run || pjcmd.trigger.state.processing == Armed || 
                                            pjcmd.iCal.state.processing == JSPARCalibrationStart || 
                                            pjcmd.iALog1.state.processing == Running || pjcmd.iALog2.state.processing == Running;

    /* ------------------------------------------------------------ */
    /*					Button Processing        					*/
    /* ------------------------------------------------------------ */

    // someone hit the button
    if(!fBtn && GetGPIO(PIN_BTN1))
    {
        // stop logging if we are logging
        if(pjcmd.iALog1.state.processing == Running) ALOGStop(&pjcmd.iALog1);
        if(pjcmd.iALog2.state.processing == Running) ALOGStop(&pjcmd.iALog2);

        tBtn = SYSGetMilliSecond();
        fBtn = true;
    }

    // debounce, if the button is pressed, refresh the timer
    else if(fBtn && GetGPIO(PIN_BTN1))
    {
        tBtn = SYSGetMilliSecond();
    }

    // if the timer expired
    else if(fBtn && (SYSGetMilliSecond() - tBtn) > 500)
    {
        fBtn = false;
    }
            
    /* ------------------------------------------------------------ */
    /*					LED Processing           					*/
    /* ------------------------------------------------------------ */

    if(fStopBlink != fNoLEDs)
    {
        fNoLEDs = fStopBlink;

        if(fNoLEDs)
        {
            SetGPIO(PIN_LED_1, 1);                  
        }
        else
        {
            SetGPIO(PIN_LED_1, 0);                  
            tLed    = SYSGetMilliSecond();
            fBlink  = false;
        }

        SetGPIO(PIN_LED_2, 0);
        SetGPIO(PIN_LED_3, 0);
        SetGPIO(PIN_LED_4, 0);    
    }

    if(fNoLEDs)
    {
        return(Idle);
    }

    // see if we are on a blink boundary
    if ((SYSGetMilliSecond() - tLed) >= 500)
    {
            tLed    = SYSGetMilliSecond();
            fBlink  = true;
    }

    // see if our bus state has changed
    if(fBlockIOBusL != fBlockIOBus)
    {
        fBlockIOBusL = fBlockIOBus;

        // going to run the Logic Analyzer
        if(fBlockIOBus)
        {
            SetGPIO(PIN_LED_1, 1);                  // say we are in the Logic Analyzer
            SetGPIO(PIN_LED_2, 0);
            SetGPIO(PIN_LED_3, 0);
            SetGPIO(PIN_LED_4, (MState == MLoop));    // say we are running
        }

        // done running the logic analyzer
        else
        {
            i   = 0;                                // reset the IP counter
            SetGPIO(PIN_LED_1, 0);
            SetGPIO(PIN_LED_2, 0);
            SetGPIO(PIN_LED_3, 0);
            SetGPIO(PIN_LED_4, MState == MLoop && GetGPIO(PIN_INT_MRF));
        }
    }

    // if the IO
    if(!fBlockIOBus && fBlink)
    {
        // if we are up and running
        if(MState == MLoop)
        {

            // if wifi is ready
            if(deIPcK.isIPReady())
            {
                uint32_t j;
                uint32_t k;

                if(ipOpenScope.u32 == 0)
                {
                    deIPcK.getMyIP(ipOpenScope);
                    ip1 = ipOpenScope.u8[3] / 100;
                    ip2 = ipOpenScope.u8[3] % 100;
                    ip3 = ip2 % 10;
                    ip2 /= 10;
                    i   = 0;
                }

                // check to see if the MRF int pin is high,
                // for the most part it should be
                else
                {
                     SetGPIO(PIN_LED_4, GetGPIO(PIN_INT_MRF));
                }

                j = i / 2;
                k = i % 2;

                if(k == 0)
                {
                    SetGPIO(PIN_LED_1, 0);
                    SetGPIO(PIN_LED_2, 0);
                    SetGPIO(PIN_LED_3, 0);
                }
                else
                {
                    if(j < ip1) SetGPIO(PIN_LED_1, 1);
                    if(j < ip2) SetGPIO(PIN_LED_2, 1);
                    if(j < ip3) SetGPIO(PIN_LED_3, 1);
                }

                if(j>=ip1 && j>=ip2 && j>=ip3) 
                {
                    i = 0;
                }
                else
                {
                    i++;
                }
            }

            // no wifi, but we are ready over COM
            else
            {
                    SetGPIO(PIN_LED_4, !GetGPIO(PIN_LED_4));
            }
        }

        // not ready
        else
        {
            ipOpenScope.u32 = 0;
            SetGPIO(PIN_LED_1, 0);
            SetGPIO(PIN_LED_2, 0);
            SetGPIO(PIN_LED_3, 0);
            SetGPIO(PIN_LED_4, 0);
        }
    }

    return(Idle);
}

