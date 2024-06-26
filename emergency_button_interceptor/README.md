# Shark Mobile Base Emergency Button Interceptor

The hoverboard's boards that SHARK MB uses, does not provide an way of interface with a emergency stop button. In order to not make hardware changes in the board itself, an interceptor using an raspberry pico w was developed. The basic structure, is that the microcontroller will interface both the computer and the hoverboard board through serial protocol (UART). The messages are received of both ends, and then retransmitted to the other. When the button GPIO is in low logic level, the microcontroller understands that the emergency button is activated, and sends a zero-speed command to the hoverboard, instead of the speed received by the computer.

## Pinout Table:

| Function | GPIO |
|----------| -----|
| Emergency Button | GPIO3 |
| Hoverboard TX | GPIO0|
| Hovearboard RX | GPIO1 |
| PC TX | GPIO4 |
| PC RX | GPIO5 |

**Observation:** This table describes the function related to the microcontroller, that means for example, that the PC TX pin, must be connected to the Computer RX data bus.