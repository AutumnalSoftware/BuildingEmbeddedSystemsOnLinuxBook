
# Generating Synthetic Sensor Data

To test the system without physical hardware, this repository provides a small utility program called *weather_tx* that generates synthetic sensor data and transmits it to the system.

The repository also provides a companion program called *weather_rx*, which can be used as a minimal standalone receiver for quick validation of transmitted data.

Both tools run on the host system.

*weather_tx* sends measurements over either UDP or a virtual UART created with socat.

*weather_rx* receives and decodes measurements and prints them to the console.

These tools allow you to validate:

* Measurement header construction

* Binary serialization (BDS)

* Endianness correctness

* Typed measurement decoding

* UDP or UART transport

* End-to-end wire integrity

These applications are simple and serve as practical debugging and validation tools.

### Building ExternalDataSource:

From the repository root:

```
mkdir build
cd build
cmake ..
cmake --build .
```

The executables will appear under:

```
build/ExternalDataSource/weather_tx
```

and

```
build/ExternalDataSource/weather_rx
```

### UDP example

Run the system under test listening on UDP port 9000, then start the transmitter:

```
weather_tx \
  --udp 127.0.0.1:9000 \
  --temp-hz 2 \
  --pressure-hz 1 \
  --humidity-hz 1
```

This sends temperature at 2 Hz and pressure and humidity at 1 Hz.

### Virtual UART example using socat

Create a pair of connected virtual serial ports:

```
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

This prints two device names, for example:

```
/dev/pts/2
/dev/pts/3
```

Configure the system to read from one end and run the transmitter on the other:

```
weather_tx \
  --uart /dev/pts/3 \
  --temp-hz 2 \
  --pressure-hz 1
```

The baud rate parameter is accepted for compatibility with real hardware, although it typically has no effect when using virtual ports.

### Duration and logging

The transmitter runs indefinitely unless a duration is specified:

```
--duration 30
```

Periodic summaries can be controlled using:

```
--log-every 5
```

### Repository Local Testing

A typical local smoke test sequence:

Terminal 1:
```
weather_rx --udp 0.0.0.0:9000
```

Terminal 2:
```
weather_tx --udp 127.0.0.1:9000 --temp-hz 2 --duration 5
```

This validates:

* Packet transmission

* Header decoding

* Payload decoding

* Typed dispatch

before integrating with higher-level system components.
