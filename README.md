# Automated Water/Wastewater Management and Irrigation System, Arduino/Embedded System Controller Source

# Screenshots

<p float="left">
  <img src="/screenshots/admin-panel-1.jpg" alt="splashscreen" width="195px">
  <img src="/screenshots/Launchlab-winners.jpg" alt="dashboard" width="195px">
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

## This Repo: Background on how the backend fits in 
* This repo is only one aspect of the project as a whole, it is the backend and installer admin panel, which uses the MQTT protocol to communicate with various systems (depending on user) and with the ionic clients (mobile apps), which are also MQTT clients. 
* The backend is reposnible for sending out scheduled MQTT messages to trigger irrigation, backwashing and draining of systems, based on settings in the system in question's app (Each system belongs to a user/family, and they can control it using their app which they log into)
* The backend also executes various other business logic functions



## Other aspects to the project 
* Ionic app which is the consumer facing client. This app is also an MQTT client and allows users to log in and observe/control their systems. From the app, users may set irrigation schedules and zones, backwash and drain schedules, and specify what type of water their garden should irrigate with (municipal, rainwater or greywater). Users may also observe their tank water levels real time, as these levels are broadcast via MQTT over GPRS from each system (using ultrasonic distance sensors in the tanks) 
* MQTT broker, we used Amazon's CloudMQTT for this, but were in the process of writing our own broker using Mosca when myself and my co-founder had the meltdown. 
* C++ scripts that ran on Arduinos (micros I think they were). Each system was controlled by an Arduino with a connected GPRS chip, the chip allowed for the Arduino's to also be MQTT clients which could talk to the apps and backend over our local cellular networks (Vodacom). The systems continuously monitor and broadcast water levels to backend over MQTT and GPRS (using ultrasonic distance sensors), so that the app can inform users of their realtime water levels. 

## Instructions
Because of all of the various parts, (backend, broker, hardware) this project will likely not work out of the box. The app should work, but will not be linked to any systems. The repo is actually more to be used as a guideline as to how we went about building the systems, so that you can use it for educational purposes - and use bits of code you may want (MQTT, weather service etc. )

## Contact
If you are interested in this project, please do not hesitate to reach out to me, and I will help you out as best I can. 
