# ESP32-FIREBASE-RTDB-REST-LOGGER — push sensor readings to Firebase over REST

IoT example by [Fluidwire](https://fluidwire.com/).

The ESP32 POSTs timestamped readings to Firebase Realtime Database using its plain HTTPS REST API — no Firebase SDK, no heavyweight dependencies. A popular backend for mobile-app thesis prototypes.

## What it does

- Connects to WiFi using credentials in `config.h`
- POSTs `{"value":..., "uptime_s":..., "ts":{".sv":"timestamp"}}` to `<db>/devices/<id>/readings.json`; Firebase adds a unique push key and a server-side timestamp
- Reads the upload interval back from `<db>/devices/<id>/config/interval_s` every 2 minutes, so you can retune the device from the Firebase console
- Prints clear messages for 401 (auth/rules) and 404 (bad host)

The default "sensor" is the ESP32's on-chip temperature, so it runs with **zero external hardware**. Replace `readSensor()` with your real sensor later.

## Required hardware

- 1x ESP32 dev board (any WiFi-capable ESP32)
- A Firebase project with a Realtime Database (free Spark plan works)

## Required libraries

- **ArduinoJson** by Benoit Blanchon (Library Manager, v7+)
- `WiFi`, `WiFiClientSecure`, `HTTPClient` ship with the **ESP32 Arduino core** (Boards Manager → `esp32` by Espressif Systems)

## Setup

1. In the [Firebase console](https://console.firebase.google.com/), create a project and a **Realtime Database**. Copy its host, e.g. `your-project-default-rtdb.firebaseio.com`.
2. Copy `config.h.example` to `config.h` and fill in WiFi, `FIREBASE_HOST` and (optionally) `FIREBASE_AUTH`.
3. For a first test, use test-mode rules (temporary!) and leave `FIREBASE_AUTH` empty. Afterwards, lock down your rules and use an auth token.
4. Optional remote config: in the console, add a number at `/devices/fluidwire-esp32-01/config/interval_s` (e.g. `10`).
5. Select **Tools → Board → ESP32 Dev Module**, pick the COM port, and **Upload**.

## Expected output

Serial Monitor, baud **115200**:

```
Fluidwire ESP32 Firebase RTDB logger
WiFi: connecting to your-wifi-ssid ... OK, ip=192.168.1.42
POST -> 200 {"name":"-Nabc123XyZ"}
Config: interval -> 10 s
POST -> 200 {"name":"-Nabc124QwE"}
```

Entries appear live under `devices/fluidwire-esp32-01/readings` in the console.

## Troubleshooting

- **`POST -> 401`** — token is wrong or your database rules deny writes. Check `FIREBASE_AUTH` and the Rules tab.
- **`POST -> 404`** — wrong `FIREBASE_HOST`. Use the host only, without `https://` or a trailing slash. Databases in other regions use `...firebasedatabase.app`.
- **`POST failed: connection refused` / timeouts** — no internet on the WiFi network, or the network blocks HTTPS.
- **Interval never changes** — the value at `interval_s` must be a plain number between 5 and 3600.
- **WiFi connect spins forever** — wrong SSID/password, or 5 GHz-only network. Non-S3 ESP32s are 2.4 GHz only.

## Security note

Don't commit `config.h`; the `.gitignore` excludes it. The sketch calls `client.setInsecure()`, which skips certificate validation — fine for development, but pin the Google root CA for production. Never leave test-mode (open) database rules in place, and prefer per-device tokens over a shared database secret.

## License

MIT — see [LICENSE](LICENSE).

## Got stuck?

Drop by [fluidwire.com](https://fluidwire.com/) — we ship IoT building blocks and are happy to help.
