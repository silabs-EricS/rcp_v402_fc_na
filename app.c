/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"

#include "sl_simple_button.h"
#include "rail.h"

bool enable = true;

extern RAIL_RxPacketHandle_t __real_RAIL_GetRxPacketInfo(RAIL_Handle_t railHandle,
                                                         RAIL_RxPacketHandle_t packetHandle,
                                                         RAIL_RxPacketInfo_t *pPacketInfo);

RAIL_RxPacketHandle_t __wrap_RAIL_GetRxPacketInfo(RAIL_Handle_t railHandle,
                                           RAIL_RxPacketHandle_t packetHandle,
                                           RAIL_RxPacketInfo_t *pPacketInfo)
{
  RAIL_RxPacketHandle_t handle = __real_RAIL_GetRxPacketInfo(railHandle,
                                                   packetHandle,
                                                   pPacketInfo);
  if (enable == false){
      pPacketInfo->packetStatus = RAIL_RX_PACKET_READY_CRC_ERROR;
  }
  return handle;
}

void sl_button_on_change  ( const sl_button_t *   handle  )
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED){
    enable = false;
  } else {
    enable = true;
  }
}

void sl_set_rx_enable(bool en)
{
  enable = en;
}
/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}
