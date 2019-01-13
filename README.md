# Automated Water/Wastewater Management and Irrigation System, Arduino/Embedded System Controller Source

# Screenshots

<p float="left">
  <img src="/screenshots/system-embedded-controller.jpg" alt="embedded-controller" width="195px">
  <img src="/screenshots/system-5.jpg" alt="embedded-controller" width="195px">
  <img src="/screenshots/system-4.jpg" alt="embedded-controller" width="195px">
  <img src="/screenshots/system-3.jpg" alt="dashboard" width="195px">
</p>


## Background: What was this project about?

This is part of a project that was a product of a company myself and a few friends started out of university. The project was an automated (Internet of Things) based Rainwater, Greywater and Irrigation Smart System. 

The system is able to smartly manage your house’s water supply by alternating between rainwater and municipal water – depending on the level of water in your tanks. It also smartly determines which water sources (Greywater, rainwater, municipal) will be used for irrigating your garden.

The system comes with a mobile app (available on iOS and android usng Ionic) that allows you to:

* Check your greywater and rainwater levels
* ‘Zone’ your garden, for irrigation
* Manage your system (backwashes, irrigation schedules etc.)

The system was also designed to be smart enough to make decisions based on the weather at your location (if it is raining, it will not irrigate). Unfortunately we never got around to fully implementing this functionality. 

Also (rather unfortunately), myself and my co-founder had a bit of a meltdown as he wanted to focus on 'non-smart' such systems, and I wanted to go ahead with this. Thus I was edged out and this smart system project left in the past. So, in short, I am open sourcing all of the code in the hope that someone may find it of interest if they are trying to do something similar - it is quite rusty as it was a version 1, but it worked well enough and we actually have 3 systems still up and running and working with the app at time of writing. 

## This Repo: Background on how the contollers fit in 
* This repo is only one aspect of the project as a whole, it is the source code for the embedded system controllers which serve as the 'brain' of the water management system physical hardware. 
* The controllers serve a number iof purposes:
  * They control relays which in turn activate and deactivate solenoids, which reroute water to where it needs to go (for irrigation, draining, backwashing etc.) 
  * They interpret tank level readings by reading ultrasonic distance sensors fitted in the tanks, and broadcast these readings to the server/backend using the MQTT protocol over GPRS
  * They allow for a direct connection to the systems themselves over the internet (GPRS/mobile networks). This is so that both the backend and the mobile (ionic) clients may observe and control the systems in real time.



## Other aspects to the project 
* Node backend which serves as another MQTT client to relay commands from the ionic clients to the systems themselves. The main business logic (scheduling for irrigation, backwashing etc.) is done here. - see https://github.com/peter-stuart-turner/iot-water-management-backend
* Ionic app which is the consumer facing client. This app is also an MQTT client and allows users to log in and observe/control their systems. From the app, users may set irrigation schedules and zones, backwash and drain schedules, and specify what type of water their garden should irrigate with (municipal, rainwater or greywater). Users may also observe their tank water levels real time, as these levels are broadcast via MQTT over GPRS from each system (using ultrasonic distance sensors in the tanks) - see https://github.com/peter-stuart-turner/iot-water-management-mobile-app 
* MQTT broker, we used Amazon's CloudMQTT for this, but were in the process of writing our own broker using Mosca when myself and my co-founder had the meltdown. - see https://www.cloudmqtt.com 

## Instructions
Because of all of the various parts, (backend, broker, hardware) this project will likely not work out of the box. The app should work, but will not be linked to any systems. The repo is actually more to be used as a guideline as to how we went about building the systems, so that you can use it for educational purposes - and use bits of code you may want (MQTT, weather service etc. )

## Contact
If you are interested in this project, please do not hesitate to reach out to me, and I will help you out as best I can. 
