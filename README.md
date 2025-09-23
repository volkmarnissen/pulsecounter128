# Pulse Counter 128 Channels

This project uses the [Kincony A16](https://www.kincony.com/esp32-s3-16-channel-gpio-module.html) board as a 128-channel Pulse Counter. It uses MQTT to send the Counts to consumer apps.

![The Koincony Board](https://www.kincony.com/images/KC868-A16v3/KC868-A16v3-1.jpg "Kincony Board")

## Features

- 128 channels organized by matrix switches
- Different frequencies for Energy Meters and Water Meters
- Simple configuration in a browser
- Ethernet and Wifi Network connection
- Buffer for Network/MQTT Broker issues

## Installation and Configuration

- Upload the Firmware
- Connect network via **ethernet** cable and DHCP
  Using the ethernet connection, the Wifi connection can be configured in the browser.
- Open browser
  The hostname of the device after uploading the firmware and connecting to Ethernet is plscounter.
  The URL for the configuration is http://plscounter [.lan]
  **This will work only one hour after startup or reset**
- Wiring:
  Connect **one output port** to the plus connector of the up to **_16 S0 interfaces_**.
  Connect the minus connector of each of the S0 interfaces to **_one of the 16 input ports_** of the kincony board.

- Configure the output ports 1-8 either as EMeter or Water Meter. This controls the frequency the
  S0 interface will be read. EMeter polls every 30ms. WaterMeter polls every ??? ms.
- Configure the MQTT broker
- Assign a mqttname to each of the Pulscounters.
- Set the divider for each pulse counter. Usually, you'll find the devider on the S0 device.
  Typical values are 400 or 1000. The default divider is 1000.
- Configure the schedule. It's controlls when to publish the results.
  E.g. if you want to update every 15 minutes, configure schedule minutes to "0, 15, 30, 45"

According to the schedule, the pulse counter will publish the pulse counts for all configured counters together with date and time to the MQTT broker. The counters will be resetted and start counting by zero.

If the network is down or the MQTT broker is not available, the results will be stored on the device. As soon as the broker is available, all results will be published.

## MQTT messages

The mqtt payload is a JSON formatted object containing the count for all configured counters.

Example:

```
{
    "datetime": 33300001000, # Unix time in milliseconds since 1970
    "yourmqttname1": 1.65, # pulsecount divided by divider
    "yourmqttname2": 1.62,
    ...
    "yourmqttnameX": 0.04
}
```

## How to use 128 Channels

For 128 Channels, you can connect up to 16 S0 interfaces plus connectors to each output port.
Connect the minus connectors of the S0 interfaces which are connected to the **_same output port_** to an **_individual input port_**
Do the same for every connected output port.
Each input port can have up to 8 attached S0 interfaces (one connection for every output port).

## Energy safing measures

The network connection of the board is normally down.
It's only when for publishing a message or after reset.

**The browser is only available after reset or startup for 1 hour after last changes of a propperty.**

## udp logging

You can configure an udp logging server in the web ui.
On a linux system you can use the command `nc -uld <port>` to monitor the logs.

As an alternative you can create a systemd service which logs into a file on the udp logging server.

On the udp logging server create a systemd service in `/etc/systemd/system/udplog.service`

It should look like this:

```
[Unit]
Description=Service to receive udp logging and store it to a file.
[Service]
Type=simple
StandardOutput=file:/var/log/plscount.log
StandardError=file:/var/log/plscount.err
ExecStart=/usr/bin/nc $SCRIPT_ARGS
Environment="SCRIPT_ARGS=-uld  3333"
User=scanner
[Install]
WantedBy=multi-user.target
```

Then you can enable and start the server

```
systemd enable udplog
systemd start udplog
systemd start udplog

```

### Planned changes

- The wiring of the Kincony board doesn't allow to use deep sleep and ULP.
  Once there is a board available which supports it, the energy consumption will drop.
