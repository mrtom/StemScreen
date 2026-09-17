# Counter protocol v1

BLE local name: `BikeStem`. The Garmin scans for the service UUID, not the name.

- Service: `93e1b4a0-5c21-4e62-a738-9d608c17f201`
- RX characteristic: `93e1b4a1-5c21-4e62-a738-9d608c17f201`
- Operation: Garmin central writes with response; Waveshare peripheral receives.
- Exactly 16 bytes per write; all multibyte integers unsigned little-endian.

Startup: submit the Garmin profile, then start scanning independently of its
registration callback. Waiting for that callback before scanning stalled on
Tom's Edge 530 software 9.75; independent scanning reached acknowledged writes.
Pairing and writes still require a matching successful registration callback.
Scanning stops after selecting the peripheral. This changes startup sequencing,
not UUIDs or wire format.

| Offset | Bytes | Value |
| --- | --- | --- |
| 0 | 2 | Magic `42 53` hexadecimal (ASCII BS) |
| 2 | 1 | Version 1 |
| 3 | 1 | Message kind 1 = diagnostic counter |
| 4 | 4 | Compute-call counter |
| 8 | 4 | Garmin Unix timestamp, seconds; diagnostic only |
| 12 | 1 | View lifecycle flag: 1 after onShow; 0 initially or after onHide |
| 13 | 3 | Reserved, all zero |

The counter wraps to zero after 2,147,483,647 on the sender. It resets when the app instance is recreated. Gaps are allowed: the sender keeps the latest counter rather than accumulating unsent packets. It keeps at most one write in flight. Five compute ticks without acknowledgement cause a connection reset; disconnected handles get about 25 ticks to reconnect before rediscovery. Retry delay is three ticks. These are compute ticks, so timeouts do not advance while Garmin suspends compute.

Byte 12 retains its original encoding and the source member name `visible`, but
it is not reliable evidence of the selected Garmin page. On Edge 530 software
9.75 it stayed 1 while the map was visible for 120 seconds. Zero also means the
initial value before any onShow, not necessarily an observed onHide. The receiver
now labels this diagnostic `View flag: set/clear`. Do not gate transmission or
infer activity timer state from it. Actual counter delivery while viewing the
map is the hidden-page test evidence.

The receiver checks size, magic, version, message kind, flag and reserved bytes. Malformed writes do not refresh the screen's data age. A BLE write acknowledgement alone is not an application-level decode acknowledgement; the board's LIVE display and serial output are the acceptance evidence.

Receiver connection state can be established by a connection event or a valid
write. On Tom's core 3.3.11 build, valid writes arrived without onConnect, so
requiring that callback rejected every packet. A valid write now restores the
connection state and cancels any pending advertising restart. Invalid writes do
neither. Both server callbacks and, on Bluedroid, the library's custom GATTS event
hook handle disconnects. Duplicate connection events do not erase fresh data.
If disconnect notification is also absent, five seconds without valid writes
still produces STALE; no local counter is substituted. This state logic lives
in ReceiverState.h and has host regression coverage.

BLE callbacks copy state under a short ESP32 critical section; the main loop owns rendering and serial output. A 204×40 pixel strip buffer uses 16,320 bytes, avoiding a full-screen internal framebuffer alongside the BLE stack. Advertising restarts after disconnection. The prototype uses a writable characteristic without authentication or bonding and selects the first matching advertised service; device selection/security are future product decisions.

Relevant Garmin API references: [BLE module](https://developer.garmin.com/connect-iq/api-docs/Toybox/BluetoothLowEnergy.html), [write API](https://developer.garmin.com/connect-iq/api-docs/Toybox/BluetoothLowEnergy/Characteristic.html#requestWrite-instance_function), [DataField lifecycle](https://developer.garmin.com/connect-iq/api-docs/Toybox/WatchUi/DataField.html).
