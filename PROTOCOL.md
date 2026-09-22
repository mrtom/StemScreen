# Ride protocol v3

The Garmin Edge 530 is the BLE central/GATT client; StemScreen is the peripheral.
BLE local name: `BikeStem`. Discovery matches the service UUID, not the name.

- Service: `93e1b4a0-5c21-4e62-a738-9d608c17f201`
- RX characteristic: `93e1b4a1-5c21-4e62-a738-9d608c17f201`
- Operation: write with response, exactly 20 bytes, unsigned little-endian integers.
- Garmin application ID: `9080770dece64c76b104c84a44d0bad1` (unchanged).

| Offset | Bytes | Value |
| --- | --- | --- |
| 0 | 2 | Magic `42 53` hexadecimal (ASCII BS) |
| 2 | 1 | Version 3 |
| 3 | 1 | Kind 2: ride snapshot |
| 4 | 1 | Validity flags: bit 0 clock, bit 1 duration, bit 2 activity state |
| 5 | 1 | State: 0 Off, 1 manually stopped, 2 Auto Paused, 3 Running |
| 6 | 2 | Garmin local year (2000–2099) |
| 8 | 1 | Month (1–12) |
| 9 | 1 | Day (validated against month/year) |
| 10 | 1 | Hour (0–23) |
| 11 | 1 | Minute (0–59) |
| 12 | 1 | Second (0–59) |
| 13 | 1 | Reserved zero |
| 14 | 4 | Whole active ride seconds, from Garmin `timerTime` |
| 18 | 2 | Fractional active ride milliseconds (0–999) |

Flags outside bits 0–2, unknown states, invalid dates/times, nonzero reserved bytes,
and fractions above 999 are rejected. Fields whose validity flag is clear must
be zero, including both duration components. Unknown state is distinct from a
valid Off state. No timezone conversion is needed: the clock fields are local.

Version 2 used the same layout but reserved bytes 18–19 as zero and discarded
fractional time. Version 1 was a 16-byte diagnostic counter. The current receiver
rejects both old versions. **Deploy sender and receiver together**; the version
bump prevents an incompatible pair from silently interpreting the new fields.

## Timer display and freshness

The receiver records `millis()` only for accepted packets. A fresh Running state
with valid duration and state enables a continuous millisecond estimate, driven
by the board's monotonic clock. New running packets preserve its fractional phase.
Errors within 150 ms are ignored. Three consecutive errors outside that deadband
in the same direction enable rate correction, limited to 10 ms per second (1%).
This suppresses isolated arrival jitter and avoids backwards steps during normal
correction. State changes, decreasing source duration, a gap of at least five
seconds, reconnects and errors of at least two seconds resynchronise immediately.
The source fraction is retained without increasing the BLE payload beyond 20 bytes.

Garmin snapshot time and BLE delivery delay are not separately measured; the
estimate can lag Garmin by transport latency. Paused/off/unknown states do not
advance. After disconnection or five seconds without a valid packet, the display
uses the last confirmed duration in grey. Rendering never refreshes packet age
or changes the confirmed packet. A new connection requires fresh data before
local advancement resumes. Arithmetic handles `millis()` wrap and saturates the
displayed duration at the wire seconds maximum instead of overflowing.

## Transport and lifecycle

`compute()` samples and transmits independently of rendering. The sender keeps
at most one write in flight, sending the latest snapshot rather than queuing old
values. Five compute ticks without acknowledgement trigger a connection reset;
disconnected handles get about 25 ticks before rediscovery, followed by a retry
delay of three ticks. These are compute ticks, not wall-clock timeouts.

Startup submits the profile then starts scanning independently of registration's
callback; waiting for that callback before scanning stalled on Tom's Edge 530.
Pairing/writes still require successful registration. Scanning stops after a
matching peripheral is selected. `onShow`/`onHide` are view lifecycle diagnostics,
not dependable page visibility or activity state; they never gate transmission.

A valid write is connection evidence even if `onConnect` is absent. Invalid
writes never change freshness or the clock estimate. Server callbacks and the
custom Bluedroid GATTS hook handle disconnects; duplicate connect events preserve
fresh data. Missing disconnect events still lead to STALE after five seconds.
The cause of previously missing callbacks on core 3.3.11 remains unestablished.

Callbacks decode/copy state and perform bounded clock arithmetic under a short
lock. All display and serial work remains in the main loop. The 204×48 strip
buffer uses 19,584 bytes. The prototype uses no authentication/bonding and selects
the first matching service. A write acknowledgement proves transport completion,
not successful packet decoding; receiver output is the acceptance evidence.

## Host checks

```sh
c++ -std=c++11 -Wall -Wextra -Werror -pedantic tests/protocol_test.cpp -o /tmp/stem-protocol-test
/tmp/stem-protocol-test
c++ -std=c++11 -Wall -Wextra -Werror -pedantic tests/ride_clock_test.cpp -o /tmp/stem-ride-clock-test
/tmp/stem-ride-clock-test
```

Garmin encoding tests are in `Garmin/tests/RideDataTest.mc`, built with
`Garmin/tests.jungle` and `monkeyc -t`, then executed in the Edge 530 simulator.
