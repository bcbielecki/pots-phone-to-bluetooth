# pots-phone-to-bluetooth
Transforming the iconic rotary phone into a modern hands-free bluetooth phone device for smartphones

## Overview
I have a fascination with old technology, which manifests as an unfettered accumulation of ~~junk~~ film cameras, typewriters, 
and now, a rotary phone. The allure of real, sturdy components, haptic, mechanical buttons, 
bells that ring, sounds that ding, and beige plastic is impossible to resist! Admittedly, the rotary phone was meant as a nice
but inoperable centerpiece to a collection of midcentury decor in my apartment. However, enough house guests play with the damn dial that I may as well try to make it functional. 

For lack of a landline connection, I needed a suitable alternative by which the phone would reach the real world.
I first considered gutting the electrical internals of the phone, meant for AC power delivery from a phone jack. The insides would then be replaced with a microcontroller which could relay calls from a cell service module. The MCU would also decode numbers dialed from the rotary. Even better, this could all be done on low voltage, DC power. Easy peasy..... but then there was the matter of the bell. The actuator which rang the bell relied on that AC power
which I had so readily scorned as unecessary for my purposes. Perhaps I could have done without it, but if I didn't have a bell to jolt myself and the neighbors five units down awake, what was the point? Better yet, if the old electrical circuits still worked, why not use them to my advantage?
And the more I thought about it, the more I realized I committed myself to a $20+/month cell plan for a phone I would use twice before forgetting.

So, I have arrived at a new, ideally final, solution: use a bluetooth-capable microcontroller to connect to my smartphone. The microcontroller would  transmit my smartphone call audio over the phone line to the rotary phone. The microcontroller would  do so, simulating the various signals which
the phone would have received some 60 years ago. A user could even dial a number on the rotary phone to trigger a call on the smartphone.

This is an **active and very incomplete** project, so if you have come across this page before its completion, good luck!
I have some notes below, outlining the project setup, but they will be rather scattered until I finish.

## References
- [ESP IoT Development Framework API Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html)
- [ArduinoBLE API Docs](https://docs.arduino.cc/libraries/arduinoble/#BLE%20class)
- [ESP32-WROOM-32D Datasheet](https://documentation.espressif.com/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf)

## Bill of Materials
- ESP32-WROOM-32D
    - Most importantly, it is a Bluetooth Low Energy (BLE) capable device.
- AT&T Bell rotary phone with a standard POTS phone line port

## Development Environment Setup

1. Install Visual Studio (VS) Code
2. Install the Platform IO Extension in VS Code


## Project Checklist

- [ ] Prototype bluetooth connection using the ESP chips, built-in BLE module
- [ ] I need to find a manual which describes the communication protocol used by the rotary phone
- [ ] Find or devise code which can decode signals from the rotary phone (like dial movements, hanging up, etc.)
- [ ] Find or devise code which can encode and send signals to the rotary phone
- [ ] Determine power requirements for the phone line


## Project Notes

- Brief research suggests I need to look at an implementation of Bluetooth's Hands-Free Profile (HFP) or Headphones Profile (HSP) for my purposes
- What is a SLIC module? Maybe that's what I need for the phone signals?