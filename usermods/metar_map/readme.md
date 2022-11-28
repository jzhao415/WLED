# Metar Map usermod

## What is METAR and Where is it from
Meteorological Terminal Air Report (METAR) is a format for reporting weather information. A METAR weather report is predominantly used by aircraft pilots, and by meteorologists, who use aggregated METAR information to assist in weather forecasting. Raw METAR is the most common format in the world for the transmission of observational weather data.

Weather stations from around the world report weather conditions every hour using the WMO approved METAR format (see WMO code manual FM-15 and FM-16). These data are centrally collected by each country and distributed internationally by WMO and other services. ADDS gets its data a variety of feeds provided by NWS.

ADDS decodes the METAR reports and saves up to seven days of data in a database. The data available in the database includes: temperature, dew point, wind speed, wind direction, visibility, clouds and ceiling, and any reported weather conditions.

https://www.aviationweather.gov/metar

## What is usermod do
This usermod allow user to create a METAR Map with individual LED represent an airport and use LED color to represent the latest METAR information for that airpot. The most common usage is to display the fly category (Flt Cat) of the airport. This usermod pulls data from aviation weather center text data server (https://www.aviationweather.gov/dataserver) every 5 mins with configured Airport ICAO codes, parse the fly category of each airport and set the corresponding LED to match the Flt Cat color.

The color legend for the Flt Cat:

CATEGORY                            |	COLOR	    |	CEILING		                |   VISIBILITY
LIFR (Low Instrument Flight Rules)	|   Magenta	    |	below 500 feet AGL	        |   and-or	less than 1 mile
IFR (Instrument Flight Rules)       |	Red	        |	500 to below 1,000 feet AGL	|   and/or	1 mile to less than 3 miles
MVFR (Marginal Visual Flight Rules) |	Blue	    |	1,000 to 3,000 feet AGL	    |   and/or	3 to 5 miles
VFR (Visual Flight Rules)           |	Green	    |	greater than 3,000 feet AGL	|   and	greater than 5 miles

## Installation 

### [TODO] update this installation
Copy `usermod_v2_example.h` to the wled00 directory.  
Uncomment the corresponding lines in `usermods_list.cpp` and compile!  
_(You shouldn't need to actually install this, it does nothing useful)_

## Dependency Setup

### DockerCompose for MQTT&NodeRed on QNAP container station
```
version: "3"

services:
  mosquitto:
    image: eclipse-mosquitto:2
    restart: always
    command: mosquitto -c /mosquitto-no-auth.conf
    volumes:
      - mosquitto-conf:/mosquitto/config
      - mosquitto-data:/mosquitto/data
    ports:
      - "1883:1883"
    networks:
      qnet-static:
        ipv4_address: 192.168.1.12
  node-red:
    image: nodered/node-red:latest
    restart: always
    environment:
      - TZ=America/New_York
    volumes:
      - node-red-data:/data
    ports:
      - "1880:1880"
    dns:
      - "1.0.0.1"
      - "1.1.1.1"
    networks:
      qnet-static:
        ipv4_address: 192.168.1.13
volumes:
  node-red-data:
  mosquitto-conf:
  mosquitto-data:
networks:  
  qnet-static:
    driver: qnet
    driver_opts:
      iface: "eth0"
    ipam:
      driver: qnet
      options:
        iface: "eth0"
      config:
        - subnet: 192.168.1.0/24
          gateway: 192.168.1.1
```
### NodeRed Workflow
```
[{"id":"f6f2187d.f17ca8","type":"tab","label":"METAR Map Feeder","disabled":false,"info":""},{"id":"3cc11d24.ff01a2","type":"comment","z":"f6f2187d.f17ca8","name":"WARNING: please check you have started this container with a volume that is mounted to /data\\n otherwise any flow changes are lost when you redeploy or upgrade the container\\n (e.g. upgrade to a more recent node-red docker image).\\n  If you are using named volumes you can ignore this warning.\\n Double click or see info side panel to learn how to start Node-RED in Docker to save your work","info":"\nTo start docker with a bind mount volume (-v option), for example:\n\n```\ndocker run -it -p 1880:1880 -v /home/user/node_red_data:/data --name mynodered nodered/node-red\n```\n\nwhere `/home/user/node_red_data` is a directory on your host machine where you want to store your flows.\n\nIf you do not do this then you can experiment and redploy flows, but if you restart or upgrade the container the flows will be disconnected and lost. \n\nThey will still exist in a hidden data volume, which can be recovered using standard docker techniques, but that is much more complex than just starting with a named volume as described above.","x":350,"y":80,"wires":[]},{"id":"852e6b0edddcac36","type":"http request","z":"f6f2187d.f17ca8","name":"","method":"GET","ret":"txt","paytoqs":"ignore","url":"https://www.aviationweather.gov/adds/dataserver_current/httpparam?datasource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=2&stationString=KLNS,KTHV,KDMW,KFDK,KHGR,KMRB,KOKV,KFRR,KJYO,KIAD,KGAI,KDCA,KCGS,KBWI,KMTN","tls":"","persist":false,"proxy":"","insecureHTTPParser":false,"authType":"","senderr":false,"headers":[],"x":270,"y":240,"wires":[["484f593c1bc55762"]],"info":"request url:\r\n\r\nhttps://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&radialDistance=50;-77.22,39.24&mostRecentForEachStation=constraint&hoursBeforeNow=1.25\r\n\r\n\r\n50miles from KFDK radius"},{"id":"0235a17da919e94c","type":"debug","z":"f6f2187d.f17ca8","name":"debug 1","active":true,"tosidebar":true,"console":false,"tostatus":false,"complete":"true","targetType":"full","statusVal":"","statusType":"auto","x":380,"y":360,"wires":[]},{"id":"30e9afd025181985","type":"inject","z":"f6f2187d.f17ca8","name":"","props":[{"p":"payload"},{"p":"topic","vt":"str"}],"repeat":"600","crontab":"","once":true,"onceDelay":0.1,"topic":"","payload":"","payloadType":"date","x":110,"y":240,"wires":[["852e6b0edddcac36"]]},{"id":"484f593c1bc55762","type":"xml","z":"f6f2187d.f17ca8","name":"response","property":"payload","attr":"","chr":"","x":440,"y":240,"wires":[["08c8a6ae03de81f1"]]},{"id":"08c8a6ae03de81f1","type":"function","z":"f6f2187d.f17ca8","name":"function 1","func":"var response = []\n\nmsg.payload.response.data[0].METAR.forEach(function (metar) {\n    Object.keys(metar).forEach(function (field) {\n        response.push({\n            topic: \"metar/\" + metar.station_id + \"/\" + field,\n            payload: metar[field][0]\n        })\n    });\n});\n\nreturn [response];","outputs":1,"noerr":0,"initialize":"","finalize":"","libs":[],"x":620,"y":240,"wires":[["925813b6c109d3e2"]]},{"id":"925813b6c109d3e2","type":"mqtt out","z":"f6f2187d.f17ca8","name":"","topic":"","qos":"","retain":"","respTopic":"","contentType":"","userProps":"","correl":"","expiry":"","broker":"29187d6ea5bac144","x":770,"y":240,"wires":[]},{"id":"dfffd571e37f382b","type":"mqtt in","z":"f6f2187d.f17ca8","name":"","topic":"metar/+/flight_category","qos":"2","datatype":"auto-detect","broker":"29187d6ea5bac144","nl":false,"rap":true,"rh":0,"inputs":0,"x":140,"y":360,"wires":[["0235a17da919e94c"]]},{"id":"fcb5e8656f2f8b1a","type":"mqtt in","z":"f6f2187d.f17ca8","name":"","topic":"metar/+/observation_time","qos":"2","datatype":"auto-detect","broker":"29187d6ea5bac144","nl":false,"rap":true,"rh":0,"inputs":0,"x":150,"y":440,"wires":[[]]},{"id":"2e08bd3254545e08","type":"inject","z":"f6f2187d.f17ca8","name":"Test","props":[{"p":"payload"},{"p":"topic","vt":"str"}],"repeat":"","crontab":"","once":false,"onceDelay":0.1,"topic":"metar/KFRR/flight_category","payload":"VFR","payloadType":"str","x":570,"y":360,"wires":[["925813b6c109d3e2"]]},{"id":"29187d6ea5bac144","type":"mqtt-broker","name":"NodeRedMQTT","broker":"192.168.1.12","port":"1883","clientid":"","autoConnect":true,"usetls":false,"protocolVersion":"4","keepalive":"60","cleansession":true,"birthTopic":"","birthQos":"0","birthPayload":"","birthMsg":{},"closeTopic":"","closeQos":"0","closePayload":"","closeMsg":{},"willTopic":"","willQos":"0","willPayload":"","willMsg":{},"userProps":"","sessionExpiry":""}]
```