# IWindows (2021)
A home automation system prototype that controls a window and its blind in function of the temperature, luminosity and humidity 

The system was controlled by an Atmega card and an ESP to get the connection by Wifi.
The control card fetches and send data through the ESP to an MQTT server hosted at our school.

We used the MQTT server to be able to monitor all the different informations from any device connected to the server. To do that we use a Node red interface that allowed us to not only read, but also to interact with the MQTT database and thus act on the card that controlled our prototype through Internet.

The system is equipped with an auto mode that can control automatically the windows and the blind in function of user defined parameters.

Moreover, there is also an alarm mode that rings when a movement is detected.

![1649322593937](https://user-images.githubusercontent.com/72990665/162164420-e8c1955e-71bf-471e-be40-48b53f50402d.jpg)

![162163505-a98c1693-0cf0-484d-8f96-b8c843202caf(1)](https://user-images.githubusercontent.com/72990665/162168144-b27a7b76-215e-4c65-8b73-1352f47a799f.png)
