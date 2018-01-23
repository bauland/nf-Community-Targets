//
// Copyright (c) 2017 The nanoFramework project contributors
// See LICENSE file in the project root for full license information.
//

#include <ch.h>
#include <hal.h>
#include <cmsis_os.h>

#include <usbcfg.h>
#include <swo.h>
#include <targetHAL.h>
#include <WireProtocol_ReceiverThread.h>
#include <LaunchCLR.h>

void BlinkerThread(void const * argument)
{
  (void)argument;

  // loop until thread receives a request to terminate
  while (!chThdShouldTerminateX()) {

      palSetPad(GPIOA, GPIOA_LED_GREEN);
      osDelay(250);
      
      palClearPad(GPIOA, GPIOA_LED_GREEN);
      osDelay(250);
  }
  
  // nothing to deinitialize or cleanup, so it's safe to return
}
osThreadDef(BlinkerThread, osPriorityNormal, 128, "BlinkerThread");

// need to declare the Receiver thread here
osThreadDef(ReceiverThread, osPriorityNormal, 2048, "ReceiverThread");

//  Application entry point.
int main(void) {

  osThreadId blinkerThreadId;
  osThreadId receiverThreadId;

  // HAL initialization, this also initializes the configured device drivers
  // and performs the board-specific initializations.
  halInit();

  // init SWO as soon as possible to make it available to output ASAP
  #if (SWO_OUTPUT == TRUE)  
  SwoInit();
  #endif

  // The kernel is initialized but not started yet, this means that
  // main() is executing with absolute priority but interrupts are already enabled.
  osKernelInitialize();
  osDelay(20);    // Let init stabilize

  // the following IF is not mandatory, it's just providing a way for a user to 'force'
  // the board to remain in nanoBooter and not launching nanoCLR

  // Since the electron board does not provide a user button we can comment the 1st level
  // if the USER button (blue one) is pressed, skip the check for a valid CLR image and remain in booter
  //if (palReadPad(GPIOC, GPIOC_BUTTON))
  //{
    // check for valid CLR image 
    if(CheckValidCLRImage((uint32_t)&__nanoImage_end__))
    {
      // there seems to be a valid CLR image
      // launch nanoCLR
      LaunchCLR((uint32_t)&__nanoImage_end__);
    }
  //}
  
  //  Initializes a serial-over-USB CDC driver.
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  // Activates the USB driver and then the USB bus pull-up on D+.
  // Note, a delay is inserted in order to not have to disconnect the cable after a reset.
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  // Creates the blinker thread, it does not start immediately.
  blinkerThreadId = osThreadCreate(osThread(BlinkerThread), NULL);

  // create the receiver thread
  receiverThreadId = osThreadCreate(osThread(ReceiverThread), NULL);

  // start kernel, after this main() will behave like a thread with priority osPriorityNormal
  osKernelStart();

  //  Normal main() thread
  while (true) {
    osDelay(500);
  }
}