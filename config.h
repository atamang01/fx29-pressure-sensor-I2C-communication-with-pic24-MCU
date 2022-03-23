
    /*
     ** config.h
     ** 
     ** "Flying PIC24" projects device configuration
     */

#include <xc.h>         // Generic PIC24 Header file.


    // See PIC24FJ128GA010 data sheet, CH24 regarding Configuration.
    // MACRO commands _CONFIG1 and _CONFIG2 are used.
    _CONFIG1(JTAGEN_OFF // disable JTAG interface           0x3FFF
            & GCP_OFF // disable general code protection  0x7FFF
            & GWRP_OFF // disable flash write protection   0x7FFF
            & ICS_PGx2 // ICSP interface (2=default)       0x7FFF
            & FWDTEN_OFF) // disable watchdog timer           0x7F7F


    _CONFIG2(IESO_OFF // two speed start up disabled      0x7FFF
            & FCKSM_CSDCMD // disable clock-swithcing/monitor  0xFFBF
            & FNOSC_PRIPLL // primary oscillator: enable PLL   0xFBFF
            & POSCMOD_XT) // primary oscillator: XT mode      0xFFFD



























