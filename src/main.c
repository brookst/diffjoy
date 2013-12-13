/*************************************************************************
 * main.c
 * diffjoy firmware
 * the diffjoy is a 1-axis HID joystick which is the differential of two
 * analog inputs
 * Author: Tim Brooks
 * based on work by Spencer Russell and Christian Starkjohann
 * ***********************************************************************/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "usbdrv.h"
#include "oddebug.h"

/*
   Pin assignment:
   PB1 = LED output (active high)

   PB3 = First analog input
   PB4 = Second analog input

   PB0, PB2 = USB data lines
   */

#define BIT_LED 1
#define ADC_0 3
#define ADC_1 2

#define UTIL_BIN4(x)        (uchar)((0##x & 01000)/64 + (0##x & 0100)/16 + (0##x & 010)/4 + (0##x & 1))
#define UTIL_BIN8(hi, lo)   (uchar)(UTIL_BIN4(hi) * 16 + UTIL_BIN4(lo))

static uchar    reportBuffer[2];    /* buffer for HID reports */
static uchar    idleRate;           /* in 4 ms units */

static unsigned int adcPrevious;
static unsigned int adcPending;
static unsigned int usbPending;
static unsigned int adc_value[2];

const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x30,                    //     USAGE (X)
    0x27, 0xff, 0xff, 0x00, 0x00,  //     LOGICAL_MAXIMUM (65535)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

/*
 * Report Format:
 *
 * BYTE0	BYTE1
 * YYYYYYYY	-----BYY
 * 76543210	-----098
 * y - axis value 0-1023
 * B - button 0-1
 */
static void buildReport(unsigned int value)
{
    reportBuffer[0] = (uchar)(value & 0xFF);
    reportBuffer[1] = (uchar)(value >> 8);
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- ADC functions ----------------------------- */
/* ------------------------------------------------------------------------- */

static void adcInit(void)
{
    ADMUX = ADC_0;                  /* Vref=Vcc, measure ADC3 */
    ADCSRA = UTIL_BIN8(1000, 0111); /* enable ADC, not free running, interrupt disable, rate = 1/128 */
}

void adcPoll(void)
{
    // If conversion is finished and we have values to collect
    if(!(ADCSRA & (1 << ADSC)) && (adcPending < 2)) {
        adc_value[adcPending] = ADC; // Read ADC value into buffer
        if(adcPending == 0){         // Read next channel
            ADMUX = ADC_1;           // Switch to channel 1
            _delay_ms(1);            // FIXME: Delay for ADC_1 read
            ADCSRA |= (1 << ADSC);   // Start conversion
        } else {
            usbPending = 1;          // Both values read, flag for a USB report
        }
        adcPending++;                // Flag next ADC or all ADCs Read
    }
    if(adcPending >= 2) {
        if(!usbPending) {            // Wait for last USB report to send
            ADMUX = ADC_0;           // Switch to channel 0
            ADCSRA |= (1 << ADSC);
            adcPending = 0;          // Flag waiting for ADC_0
        }
    }
}

/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */

uchar usbFunctionSetup(uchar data[8])
{
    usbRequest_t    *rq = (void *)data;

    usbMsgPtr = reportBuffer;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT)
        {  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            buildReport(0);
            return sizeof(reportBuffer);
        }
        else if(rq->bRequest == USBRQ_HID_GET_IDLE)
        {
            usbMsgPtr = &idleRate;
            return 1;
        }
        else if(rq->bRequest == USBRQ_HID_SET_IDLE)
        {
            idleRate = rq->wValue.bytes[1];
        }
    }
    else
    {
        /* no vendor specific requests implemented */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */

int main(void)
{
    usbPending = 0;
    adcPending = 0;
    adcPrevious = 0;
    adc_value[0] = 0;
    adc_value[1] = 0;

    /* Calibrate the RC oscillator to 8.25 MHz. The core clock of 16.5 MHz is
     * derived from the 66 MHz peripheral clock by dividing. We assume that the
     * EEPROM contains a calibration value in location 0. If no calibration value
     * has been stored during programming, we offset Atmel's 8 MHz calibration
     * value according to the clock vs OSCCAL diagram in the data sheet. This
     * seems to be sufficiently precise (<= 1%).
     */
    uchar calibrationValue = eeprom_read_byte(0);
    if(calibrationValue != 0xff)
    {
        OSCCAL = calibrationValue;  /* a calibration value is supplied */
    }
    else
    {
        /* we have no calibration value, assume 8 MHz calibration and adjust from there */
        if(OSCCAL < 125)
        {
            OSCCAL += 3;    /* should be 3.5 */
        }
        else if(OSCCAL >= 128)
        {
            OSCCAL += 7;    /* should be 7 */
        }
        else
        {  /* must be between 125 and 128 */
            OSCCAL = 127;   /* maximum possible avoiding discontinuity */
        }
    }
    odDebugInit();
    DDRB = (1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT);
    PORTB = 0;          /* indicate USB disconnect to host */
    int i;
    for(i=0;i<20;i++)
    {  /* 300 ms disconnect, also allows our oscillator to stabilize */
        _delay_ms(15);
    }
    DDRB = 1 << BIT_LED;    /* output for LED */
    wdt_enable(WDTO_1S);
    adcInit();
    usbInit();
    sei();

    while(1) {    /* main event loop */
        wdt_reset();
        usbPoll();
        /* if a new value is ready and the last value was sent */
        if(usbPending && usbInterruptIsReady()) {
            buildReport(adc_value[0]);    // FIXME: Output just ADC2 for now
            usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            usbPending = 0;
        }
        /* if the last measurement has been handed to the USB driver */
        if(!usbPending) {
            adcPoll();
            if(adc_value[1] == 0) {       // FIXME: Check if ADC2 is locked to 0
                PORTB |= 1 << BIT_LED;    /* turn on LED */
            } else {
                PORTB &= ~(1 << BIT_LED); /* turn off LED */
            }
        }
    }
    return 0;
}
