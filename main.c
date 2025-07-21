/*
 * ---------------------------------------------------------------------------
 *  Project    : USB-MIDI Interface for TS4x I3 Application
 *  Version    : 3.1.0
 *  Target MCU : PIC18F2550
 *  Compiler   : Microchip C18
 *  Author     : BM (BM8725) for www.imidi.ro
 *  Date       : May 2025
 *  Copyright  : B Marius 2025 all rights reserved for www.imidi.ro and www.imidi.co.uk
 * ---------------------------------------------------------------------------
 *  Description:
 *    USB-MIDI Class Compliant Device (Interface for TS4x App)
 *    Designed for fast, reliable, and stable USB-MIDI communication.
 * 
 *    Core Features:
 *    - USB 2.0 Compliant (Full Speed - 12 Mbps)
 *    - Class-compliant MIDI interface (no driver needed on Windows/macOS/Linux/Android)
 *    - Supports: Control, Bulk, Interrupt, and Isochronous transfers
 *    - Bootloader-capable (optional HID/MCHP USB Bootloader)
 *    - LED activity indicators (MIDI IN/OUT/USB status)
 *    - Designed for low-latency MIDI communication
 *    - Stable 48 MHz USB clock with internal PLL
 * 
 *    Usage:
 *    - Plug-and-play USB MIDI for TS4x-based synths or controllers
 *    - Compatible with DAWs (Ableton, Cubase, Reaper, etc.)
 *    - MIDI event filtering or routing can be added in firmware
 * 
 *    Notes:
 *    - Hardware should use a proper USB termination and decoupling
 *    - Device descriptor includes TS4x I3 branding
 * ---------------------------------------------------------------------------
 *  Version History:
 *    v1.0.0 - Dec 2024 - Initial working prototype, basic MIDI IN support
 *    v2.0 -   Feb 2025 - Added MIDI OUT, endpoint optimization, improved latency
 *    v2.5 -   Apr 2025 - Bootloader integration (USB HID & MCHP), descriptor cleanup
 *    v3.1.0 - May 2025 - Final stable release, LED feedback added, cleaned structure, sysex support
 * ---------------------------------------------------------------------------
 */



#include <p18cxxx.h>            // cpu include 

#pragma config PLLDIV = 3       // 12 MHz / 3 = 4 MHz for PLL
#pragma config CPUDIV = OSC1_PLL2 // CPU clock = 96 MHz / 2 = 48 MHz
#pragma config USBDIV = 2       // USB clock = 96 MHz / 2 = 48 MHz
#pragma config FOSC = HSPLL_HS  // Cristal extern + PLL
#pragma config VREGEN = ON      // Regulator USB ON
#pragma config WDT = OFF        // Watchdog timer OFF
#pragma config LVP = OFF        // Low voltage programming OFF


#include "USB\usb.h"
#include "usb_config.h"
#include "usb_function_midi.h"
#include "hardwareprofile.h"
#include "bootloaders.h"
#include "uart.h"
#include "multitimer.h"



#define  CABLENUM  0    //

#pragma udata USB_VARIABLES = USBBUFFERADDR

/* important feature */
unsigned char UsbRecBuffer1[64];        // USB -> MIDI
unsigned char UsbRecBuffer2[64];        // USB -> MIDI

USB_AUDIO_MIDI_EVENT_PACKET MidiData;   // 
USB_AUDIO_MIDI_EVENT_PACKET CommonMidiData;// 

#pragma udata

// unsigned char MidiRecBuffer[64];     // USB -> MIDI csomag m�solat

USB_HANDLE USBTxHandle = 0;
USB_HANDLE USBRxHandle = 0;

USB_VOLATILE BYTE msCounter;

volatile unsigned char ActSenzTime = 0;

#ifdef   __18F14K50_H
rom char progID[] = {"TS4x USB-Midi by www.imidi.ro"};
#endif
#ifdef   __18F2550_H
rom char progID[] = {"TS4x USB-Midi by www.imidi.ro"};
#endif


/** PRIVATE PROTOTYPES *********************************************/
void InitializeSystem(void);
void UsbToMidiProcess(void);
void MidiToUsbProcess(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);

#define TRIS_(p, m)            TRIS ## p ## bits.TRIS ## p ## m
#define LAT_(p, m)             LAT ## p ## bits.LAT ## p ## m
#define PORT_(p, m)            PORT ## p ## bits.R ## p ## m

#define IOIN(x)                x(TRIS_) = 1
#define IOOUT(x)               x(TRIS_) = 0
#define SET(x)                 x(LAT_) = 1
#define CLR(x)                 x(LAT_) = 0
#define GET(x)                 x(PORT_)

#pragma code

/******************************************************************************
 * Function:                                     void YourHighPriorityISRCode()
 *****************************************************************************/
#pragma interrupt YourHighPriorityISRCode
void YourHighPriorityISRCode()
{
  #if defined(USB_INTERRUPT)
  USBDeviceTasks();
  #endif

  UartRxIntProcess();
  UartTxIntProcess();

  if(INTCONbits.TMR0IF)
  {
    Timer0Compless();
    Timer0IrqAck();
    ActSenzTime = 1;
  }
} //This return will be a "retfie fast", since this is in a #pragma interrupt section

/******************************************************************************
 * Function:                                           YourLowPriorityISRCode()
 *****************************************************************************/
#pragma interruptlow YourLowPriorityISRCode
void YourLowPriorityISRCode()
{
} //This return will be a "retfie", since this is in a #pragma interruptlow section



/******************************************************************************
 * Function:                                                    void main(void)
 *****************************************************************************/
void main(void)
{
  InitializeSystem();

  #if defined(USB_INTERRUPT)
  USBDeviceAttach();
  #endif

  // #include "debugtestdata.h"

  while(1)
  {
    #if defined(USB_POLLING)
    // Check bus status and service USB interrupts.
    USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
    #endif

    // User Application USB tasks
    if((USBDeviceState == CONFIGURED_STATE) && (!USBSuspendControl))
    { // USB �zemben van
      SET(LEDUSB);
      UsbToMidiProcess();
      MidiToUsbProcess();
    }
    else
      CLR(LEDUSB);

  }//end while
}//end main
.............rest is silence 
