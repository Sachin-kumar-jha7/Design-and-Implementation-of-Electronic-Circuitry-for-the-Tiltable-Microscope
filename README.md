# Design-and-Implementation-of-Electronic-Circuitry-for-the-Tiltable-Microscope
Due to the increasing application of colloids in various fields like medicine,
material science, environmental science, etc, it is important to understand the behavior and
characteristics of colloidal particles. This project aims to develop a tilting mechanism for a
microscope to enable detailed observation of such behavior. 

A DC motor controlled by an ESP32 microcontroller and an L293D driver is
used to tilt the microscope. The BNO055 IMU sensor provides real-time pitch angle feedback.
PWM signals are adjusted dynamically to control motor speed and direction based on feedback.

# Results 
1. Angular velocity -> less than 1 rpm
2. Input angle range -> -80° to +80°
3. Resolution -> 0.5°
4. Applied safety measures and brake mechanism.

However, the targeted precision of less than 0.1° and PCB implementation could not be fully achieved due to time and design limitations. Nonetheless, the current prototype lays a strong foundation for future improvements in accuracy and integration.
