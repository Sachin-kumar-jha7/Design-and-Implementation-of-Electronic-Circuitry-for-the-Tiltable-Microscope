# Design-and-Implementation-of-Electronic-Circuitry-for-the-Tiltable-Microscope
The objective of this project is to develop a motorized rotating mechanism that allows the microscope to rotate smoothly to the desired input angle in the range of-80° to +80° with an angular velocity of less than 1 rpm with a resolution of 0.5°.


Due to the increasing application of colloids in various fields like medicine,
material science, environmental science, etc, it is important to understand the behavior and
characteristics of colloidal particles. This project aims to develop a tilting mechanism for a
microscope to enable detailed observation of such behavior. The objective was to achieve precise
angular positioning of the microscope using a sensor-based control system and to implement the
design on a PCB. A DC motor controlled by an ESP32 microcontroller and an L293D driver is
used to tilt the microscope. The BNO055 IMU sensor provides real-time pitch angle feedback.
PWM signals are adjusted dynamically to control motor speed and direction based on feedback.
The results shows smooth and controlled tilting, achieving a precision of less than 1°, supported
by the effective braking mechanism as well. However, the targeted precision of less than 0.1° and
PCB implementation could not be fully achieved due to time and design limitations.
Nonetheless, the current prototype lays a strong foundation for future improvements in accuracy
and integration.
