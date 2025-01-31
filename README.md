# ThermOptimus-Smart-Temperature-Control-System

🔥❄️ ThermOptimus: Smart Temperature Control System ❄️🔥
The ThermOptimus project was designed to monitor and efficiently regulate ambient temperature using a dedicated sensor, leveraging the power of the STM32F3DISCOVERY microcontroller.

To provide instant visual feedback, the system includes LED indicators that signal three different conditions: too hot 🔴, too cold 🔵, or within the optimal range ✅.

Beyond visual alerts, the system features a cooling fan that automatically activates when the temperature exceeds a predefined threshold. The fan speed is dynamically controlled using PWM (Pulse Width Modulation)⚡, with a duty cycle that adjusts based on a threshold-based control system, ensuring proportional and efficient cooling.

Hardware Implementation
In addition to the STM32F3DISCOVERY, the project required the use of an Arduino board to provide a stable 5V power supply to the system. Furthermore, for the physical implementation of fan and LED control, a board with transistors and resistors was integrated, ensuring proper current handling and signal switching.

Software Development
The project was developed in C, utilizing the STM32’s capabilities to ensure high performance and precise temperature control.

A simple yet effective system, designed to guarantee comfort and thermal stability in any environment! ❄️🔥