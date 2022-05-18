#ifndef SL_BTCTRL_HCI_PACKET_H
#define SL_BTCTRL_HCI_PACKET_H

#include "em_common.h"

#define hci_command_header_size       3   // opcode (2 bytes), length (1 byte)
#define hci_acl_data_header_size      4   // handle (2 bytes), length (2 bytes)
#define hci_flow_control_config_index 10
#define hci_max_payload_size          256
#define hci_max_support_connection    6 

#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL

/* OpCode Group Fields */
#define BT_OGF_LINK_CTRL                        0x01
#define BT_OGF_BASEBAND                         0x03
#define BT_OGF_INFO                             0x04
#define BT_OGF_STATUS                           0x05
#define BT_OGF_LE                               0x08
#define BT_OGF_VS                               0x3f
/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)                         ((ocf) | ((ogf) << 10))

SL_PACK_START(1)
typedef struct {
  uint16_t opcode; /* HCI command opcode */
  uint8_t param_len; /* command parameter length */
  uint8_t payload[hci_max_payload_size];
} SL_ATTRIBUTE_PACKED hci_command_t;
SL_PACK_END()

SL_PACK_START(1)
typedef struct {
  uint16_t conn_handle; /* ACL connection handle */
  uint16_t length; /* Length of packet */
  uint8_t payload[hci_max_payload_size];
} SL_ATTRIBUTE_PACKED acl_packet_t;
SL_PACK_END()

enum hci_packet_type {
  hci_packet_type_ignore = 0, /* 0 used to trigger wakeup interrupt */
  hci_packet_type_command = 1,
  hci_packet_type_acl_data = 2,
  hci_packet_type_syhc_data = 3,
  hci_packet_type_event = 4,
  hci_packet_type_iso_data = 5
};

enum hci_packet_state {
  hci_packet_state_read_packet_type = 0,
  hci_packet_state_read_header = 1,
  hci_packet_state_read_data = 2
};

enum hci_packet_opcode {
  HCI_Read_Local_Supported_Commands            = BT_OP(BT_OGF_INFO, 0x0002),
  HCI_Set_Host_Controller_To_Host_Flow_Control = BT_OP(BT_OGF_BASEBAND, 0x0031),
  HCI_Host_Buffer_Size                         = BT_OP(BT_OGF_BASEBAND, 0x0033),
  HCI_Host_Number_Of_Completed_Packets         = BT_OP(BT_OGF_BASEBAND, 0x0035)
};

enum hci_packet_event {
  HCI_Disconnection_Complete                    = 0x05,
  HCI_Command_Complete                          = 0x0E
};

SL_PACK_START(1)
typedef struct {
  uint8_t  enable;
} SL_ATTRIBUTE_PACKED hci_controller_to_host_flow_control_t;
SL_PACK_END()

SL_PACK_START(1)
typedef struct {
  uint16_t acl_mtu;
  uint8_t  sco_mtu;
  uint16_t acl_pkts;
  uint16_t sco_pkts;
} SL_ATTRIBUTE_PACKED hci_host_buffer_t;
SL_PACK_END()

SL_PACK_START(1)
typedef struct {
  uint8_t  handle_num;
  uint16_t connection_handle[1];
  uint16_t count[1];
} SL_ATTRIBUTE_PACKED hci_host_completed_t;
SL_PACK_END()

SL_PACK_START(1)
typedef struct {
  uint8_t eventcode; /* HCI event code */
  uint8_t param_len; /* event parameter length */
  enum hci_packet_type packet_type : 8;
  uint16_t opcode;
  uint8_t status;
  uint8_t payload[hci_max_payload_size];
} SL_ATTRIBUTE_PACKED hci_evt_t;
SL_PACK_END()

SL_PACK_START(1)
typedef struct {
  enum hci_packet_type packet_type : 8;
  union {
    hci_command_t hci_cmd;
    hci_evt_t hci_evt;
    acl_packet_t acl_pkt;
  };
} SL_ATTRIBUTE_PACKED hci_packet_t;
SL_PACK_END()

void sl_btctrl_hci_packet_read(void);

#endif // SL_BTCTRL_HCI_PACKET_H
