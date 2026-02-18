That’s exactly the right move for the book.

Chapter 11’s purpose is **integrating external inputs**, not teaching how to write a transmitter tool. Showing the code would just dilute the main thread.

A short, practical sidebar keeps readers focused and still lets them use the tool immediately.

Here’s a book-ready sidebar you can drop in with minimal editing.

---

## Sidebar: Generating Synthetic Sensor Data

To test the system without physical hardware, the repository includes a small utility program called `weather_tx` that generates synthetic sensor data and transmits it to the system.

This tool runs on the host and sends measurements over either UDP or a virtual UART created with `socat`.

### Building the transmitter

From the repository root:

```
mkdir build
cd build
cmake ..
cmake --build .
```

The executable will appear under:

```
build/ExternalDataSource/weather_tx
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
/dev/pts/5
/dev/pts/6
```

Configure the system to read from one end and run the transmitter on the other:

```
weather_tx \
  --uart /dev/pts/6 \
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

---

This keeps the focus where it belongs: the system architecture, not tooling internals.

Next step in Chapter 11 is where things get interesting: data starts flowing into the reactor and framing path.
