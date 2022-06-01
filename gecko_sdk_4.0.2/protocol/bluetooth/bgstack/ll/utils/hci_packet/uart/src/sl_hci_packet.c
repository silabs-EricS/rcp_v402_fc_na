#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <em_device.h>
#include <em_gpio.h>
#include <em_core.h>
#include "sl_hci_common_transport.h"
#include "sl_hci_uart.h"
#include "sl_btctrl_hci_packet.h"

#define RX_BUFFER_LEN 64

SL_ALIGN(4)
static uint8_t hci_rx_buffer[RX_BUFFER_LEN] SL_ATTRIBUTE_ALIGN(4);
/* buffer pointer for transferring bytes to hci_rx_buffer */
static uint8_t *buf_byte_ptr;
static hci_packet_t *const PACKET = (hci_packet_t *) hci_rx_buffer;
static enum hci_packet_state state;
static uint16_t bytes_remaining; // first message byte contains packet type
static uint16_t total_bytes_read;
static uint16_t buffer_remaining;

#ifdef ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
static uint8_t acl_packet_support_counter;
static hci_controller_to_host_flow_control_t flow_control;
static hci_host_buffer_t host_buffer;
static hci_host_completed_t host_completed;

extern void sl_set_rx_enable(bool en);
#endif

static void reset(void)
{
  memset(hci_rx_buffer, 0, RX_BUFFER_LEN);
  state = hci_packet_state_read_packet_type;
  buf_byte_ptr = hci_rx_buffer;
  bytes_remaining = 1;
  total_bytes_read = 0;
  buffer_remaining = RX_BUFFER_LEN;
}

static void reception_failure(void)
{
  hci_common_transport_receive(NULL, 0, false);
  reset();
}

/**
 *  Called from sl_service_process_event(). Immediately returns if no data
 *  available to read. Reading of data consists of three phases: packet type,
 *  header, and data. The amount of data read during each phase is dependent
 *  upon the command. For a given phase, an attempt is made to read the full
 *  amount of data pertaining to that phase. If less than this amount is in
 *  the buffer, the amount read is subtracted from the remaining amount and
 *  function execution returns. When all data for a phase has been read, the
 *  next phase is started, or hci_common_transport_receive() is called if all
 *  data has been read.
 */
void sl_btctrl_hci_packet_read(void)
{
  uint16_t bytes_read;
  uint16_t len;

  /* Check if data available */
  if (sl_hci_uart_rx_buffered_length() <= 0) {
    return;
  }

  if (bytes_remaining >= buffer_remaining) {
    len = buffer_remaining;
  } else {
    len = bytes_remaining;
  }

  bytes_read = sl_hci_uart_read(buf_byte_ptr, len);
  buf_byte_ptr += bytes_read;
  total_bytes_read += bytes_read;
  bytes_remaining -= bytes_read;
  buffer_remaining -= bytes_read;

  if (bytes_remaining == 1 && state == hci_packet_state_read_data) {
    sl_hci_disable_sleep(true);
  }

  if (bytes_remaining > 0 && buffer_remaining > 0) {
    return;
  }

  switch (state) {
    case hci_packet_state_read_packet_type:
    {
      switch (PACKET->packet_type) {
        case hci_packet_type_ignore:
        {
          reset();
          return;
        }
        case hci_packet_type_command:
        {
          bytes_remaining = hci_command_header_size;
          break;
        }
        case hci_packet_type_acl_data:
        {
          bytes_remaining = hci_acl_data_header_size;
          break;
        }
        default:
        {
          reception_failure();
          return;
        }
      }

      state = hci_packet_state_read_header;
      break;
    }
    case hci_packet_state_read_header:
    {
      switch (PACKET->packet_type) {
        case hci_packet_type_command:
        {
          bytes_remaining = PACKET->hci_cmd.param_len;
          break;
        }
        case hci_packet_type_acl_data:
        {
          bytes_remaining = PACKET->acl_pkt.length;
          break;
        }
        default:
        {
          reception_failure();
          return;
        }
      }

      if (bytes_remaining == 0) {
        hci_common_transport_receive(hci_rx_buffer, total_bytes_read, true);
        reset();
        return;
      } else {
        state = hci_packet_state_read_data;
      }
      break;
    }
    case hci_packet_state_read_data:
    {
      // TODO: if HCI flow control related command is received, process it here, and DO NOT forward it to the controller
      //         * if HCI_Set_Host_Controller_To_Host_Flow_Control (OGF 0x03 OCF 0x0031) is received,
      //             * enable/disable flow control based on the least significant bit (0x01) of Flow_Control_Enable
      //             * if flow control is disabled, free the local buffer
      //             * send HCI_Command_Complete event
      //         * if HCI_Host_Buffer_Size message (OGF 0x03 OCF 0x0033) is received, then
      //             * set the host_buffer_slot_size to Host_ACL_Data_Packet_Length
      //             * set the host_total_buffer_size to Host_Total_Num_ACL_Data_Packets
      //             * set the host_available_buffer_size to host_total_buffer_size
      //             * init local buffer with slot size set to host_buffer_slot_size + 4 (header)
      //             * send HCI_Command_Complete event
      //         * if HCI_Host_Number_Of_Completed_Packets (OGF 0x03 OCF 0x0035) is received, then
      //             * increase the host_available_buffer_size according to Host_Num_Completed_Packets
      //                 * ideally this should be set separately for each HCI connection handle, but in first round we can use a
      //                       common host_available_buffer_size, assuming that there is only one connection
      //             * transmit as many packets from the controller buffer as possible

      if (bytes_remaining > 0) {
        hci_common_transport_receive(hci_rx_buffer, RX_BUFFER_LEN, false);
        buffer_remaining = RX_BUFFER_LEN;
        buf_byte_ptr = hci_rx_buffer;
      } else {
        sl_hci_disable_sleep(false);

        #ifdef ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
        if (PACKET->packet_type == hci_packet_type_command &&
          (PACKET->hci_cmd.opcode == HCI_Set_Host_Controller_To_Host_Flow_Control ||
           PACKET->hci_cmd.opcode == HCI_Host_Buffer_Size ||
           PACKET->hci_cmd.opcode == HCI_Host_Number_Of_Completed_Packets)) {
          switch (PACKET->hci_cmd.opcode){
            case HCI_Set_Host_Controller_To_Host_Flow_Control:
            {
              memset(&flow_control, 0, sizeof(hci_controller_to_host_flow_control_t));
              if(PACKET->hci_cmd.param_len == sizeof(hci_controller_to_host_flow_control_t)) //Prevent the overflow
                memcpy(&flow_control, PACKET->hci_cmd.payload, PACKET->hci_cmd.param_len);
              if(flow_control.enable == 0){
                acl_packet_support_counter = 0;
              }
              break;
            }
            case HCI_Host_Buffer_Size:
            {
              memset(&host_buffer, 0, sizeof(hci_host_buffer_t));
              if(PACKET->hci_cmd.param_len == sizeof(hci_host_buffer_t)) //Prevent the overflow
                memcpy(&host_buffer, PACKET->hci_cmd.payload, PACKET->hci_cmd.param_len);
              acl_packet_support_counter = host_buffer.acl_pkts;
              break;
            }
            case HCI_Host_Number_Of_Completed_Packets:
            {
              memset(&host_completed, 0, sizeof(hci_host_completed_t));
              if(PACKET->hci_cmd.param_len == sizeof(hci_host_completed_t)) //Prevent the overflow
                memcpy(&host_completed, PACKET->hci_cmd.payload, PACKET->hci_cmd.param_len);

              for(uint8_t i = 0; i < host_completed.handle_num; i++){
                acl_packet_support_counter += host_completed.count[i];
              }
              if(acl_packet_support_counter > host_buffer.acl_pkts)
                acl_packet_support_counter = host_buffer.acl_pkts;

              hci_common_transport_receive(NULL, 0, false);
              reset();
              return;
            }
            default:
              break;
          }
        }
        #endif

        hci_common_transport_receive(hci_rx_buffer, RX_BUFFER_LEN - buffer_remaining, true);
        reset();

      }
      break;
    }
    default:
    {
      reception_failure();
      return;
    }
  }
}

uint32_t hci_common_transport_transmit(uint8_t *data, int16_t len)
{
  #ifdef ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
  //TODO: check the packet type.
  //        * if it is an event, then we can simply transmit it.
  //           * if a HCI_Disconnection_Complete event was sent out, then the host_available_buffer_size should be set to host_total_buffer_size (reset)
  //        * If it is ACL data, then we should check if there is available slot on the host
  //           * if there is available slot on the host, transmit the message and decrease the host_available_buffer_size
  //           * if there is no available slot, put the message into the local buffer
  //           * in both cases, check if the length (excluding the 4 byte header!) is greater than the host_buffer_slot_size, and if it is,
  //                then fragment the data into smaller chunks, so that they fit into one slot

  hci_packet_t transmit_data;
  memcpy(&transmit_data, data, len);
  if (transmit_data.packet_type == hci_packet_type_event) {
    if (transmit_data.hci_evt.eventcode == HCI_Disconnection_Complete) {
      //While disconnect, regard all buffer in host side as release state.
      //In multiple connection use case, all connection share same host buffer(packet count).
      //Only one connection drop then regard all buffer in host side as release state, this should be a risk.
      acl_packet_support_counter = host_buffer.acl_pkts;
      sl_set_rx_enable(true); //enable it for next round
    }

    if(transmit_data.hci_evt.eventcode == HCI_Command_Complete){
      if (transmit_data.hci_evt.opcode == HCI_Read_Local_Supported_Commands) {
        transmit_data.hci_evt.payload[hci_flow_control_config_index] |= 0xE0;  //Set bit5,6,7
      }else if((transmit_data.hci_evt.opcode == HCI_Set_Host_Controller_To_Host_Flow_Control)
             ||(transmit_data.hci_evt.opcode == HCI_Host_Buffer_Size)
             /*||(transmit_data.hci_evt.opcode == HCI_Host_Number_Of_Completed_Packets)*/){
        transmit_data.hci_evt.status = 0x00; //Reply the host that controller support flow control
      }
      memcpy(data, &transmit_data, len);
    }
  } else if (transmit_data.packet_type == hci_packet_type_acl_data){
    //uint16_t ACL_packet_total_len = ((hci_packet_t *)data)->acl_pkt.length;
    // fragment the packet into chunks and then either transmit the chunks or put them into the local buffer

    if (flow_control.enable){
        //Limit on Host_ACL_Data_Packet_Length,fragment the packet in next step
        if(len > host_buffer.acl_mtu)
          return -1;

      //Limit on Host_Total_Num_ACL_Data_Packets
      if(acl_packet_support_counter > 0){
        sl_set_rx_enable(true);
        acl_packet_support_counter--;
      }else{
        sl_set_rx_enable(false);
        //Stop RX here is not good, just need to find the entry point that run out of Link Layer buffer
        //Here need buffer for the packet, then it can re-send while host side buffer released.
        return -1;
      }

    }
  }
  #endif

  return sl_hci_uart_write(data, len);
}

void hci_common_transport_init(void)
{
  reset();
  sl_hci_uart_init();
}
