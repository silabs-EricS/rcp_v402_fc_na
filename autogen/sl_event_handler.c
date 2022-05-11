#include "sl_event_handler.h"

#include "em_chip.h"
#include "sl_device_init_nvic.h"
#include "sl_board_init.h"
#include "sl_device_init_hfrco.h"
#include "sl_device_init_hfxo.h"
#include "sl_device_init_lfxo.h"
#include "sl_device_init_clocks.h"
#include "sl_device_init_emu.h"
#include "pa_conversions_efr32.h"
#include "sl_rail_util_pti.h"
#include "sl_rail_util_rf_path.h"
#include "sl_board_control.h"
#include "sl_btctrl_linklayer.h"
#include "gpiointerrupt.h"
#include "sl_mbedtls.h"
#include "sl_simple_button_instances.h"
#include "sl_uartdrv_instances.h"
#include "sl_btctrl_hci_packet.h"
#include "sl_hci_common_transport.h"
#include "sl_btctrl_hci.h"

void sl_platform_init(void)
{
  CHIP_Init();
  sl_device_init_nvic();
  sl_board_preinit();
  sl_device_init_hfrco();
  sl_device_init_hfxo();
  sl_device_init_lfxo();
  sl_device_init_clocks();
  sl_device_init_emu();
  sl_board_init();
}

void sl_driver_init(void)
{
  GPIOINT_Init();
  sl_simple_button_init_instances();
  sl_uartdrv_init_instances();
}

void sl_service_init(void)
{
  sl_board_configure_vcom();
  sl_mbedtls_init();
}

void sl_stack_init(void)
{
  sl_rail_util_pa_init();
  sl_rail_util_pti_init();
  sl_rail_util_rf_path_init();
  sl_bt_controller_init();
}

void sl_internal_app_init(void)
{
  hci_common_transport_init();
  sl_bthci_init_upper();
  sl_bthci_init_vs();
  sl_btctrl_hci_parser_init_conn();
  sl_btctrl_hci_parser_init_adv();
}

void sl_platform_process_action(void)
{
}

void sl_service_process_action(void)
{
  sl_btctrl_hci_packet_read();
}

void sl_stack_process_action(void)
{
}

void sl_internal_app_process_action(void)
{
}

