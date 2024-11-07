// connnet to mosquitto
mosquitto -c /etc/mosquitto/conf.d/mosquitto.conf -v  

// find the location of .conf
sudo find / -type f -name "mosquitto.conf" 2>/dev/null

//In the conf
listener 1883
protocol mqtt
listener 8083
protocol websockets

allow anonymous true

//Usage 
Open the Server
    ssh root@sp.grifcc.top
    ./main
Open the app

Run the ./collect

