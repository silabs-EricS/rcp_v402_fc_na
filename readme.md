# RCP With Contorller to Host HCI Flow Control

The example try to implement contorller to host HCI flow control.

## Implementation Plan
<ol>
  <li> Reply read supported command from host side with support contorller to host HCI flow control, by set highest 3 bits of octet 10. </li>
  <li> Check host side setting commmand, get information like max support packet size, check in sl_btctrl_hci_packet_read() -> hci_packet_state_read_data</li>
  <li> Check the packet counter in hci_common_transport_transmit(), then implement flow control according to the counter. Also stop RX on link layer while host buffer is used up</li>
</ol>

## Implement Status
The fundamental logic function should work.

## Know Issue
<ol>
  <li> Connection handle does not support, this should need modification on host side application, to support multiple connection </li>
  <li> The NACK on link layer is CRC error basec, so only support only one connection</li>
  <li> NACK in hci_common_transport_transmit() is not a good solution, but need to find where should be the better entry point</li>
  <li> After NACK, the packet under to send will drop, need buffer for resend, but still not know how to re-send the packet</li>
  <li> It block ACL data if size bigger than supported size, it should fragment the packet</li>
</ol>