# MapZone Alert View SDK — iOS

This SDK is provided only for Vietmap MAPs API enterprise customers. Contact your Vietmap account manager for access or [Vietmap Solutions](https://zalo.me/3189066936017422854) Zalo OA if you are interested in becoming a customer.

**Route-based speed alert.** You hand the SDK **one full route polyline** (the geometry of a navigation route, encoded `1e6`). The native engine splits it into short segments, fetches speed limits + signs for each from the Vietmap service on demand, and — on every GPS frame — hands you back speed-sign images and voice cues as the driver progresses along that route.

## Installation

The SDK ships as a binary `AlertViewSDK.xcframework`, distributed via **CocoaPods**.

**1. Add to your `Podfile`:**

```ruby
platform :ios, '13.0'
use_frameworks!

target 'YourApp' do
  pod 'MapZoneAlertView', '~> 1.0.2'
end
```

**2. Install and open the workspace:**

```bash
pod install
open YourApp.xcworkspace
```

**3. Import the module in Swift:**

```swift
import AlertViewSDK
```

> **Requirements:** deployment target **iOS 13.0** or higher; Swift 5. The `pod`
> is named `MapZoneAlertView`; the imported Swift module is `AlertViewSDK`. It
> ships as a binary `xcframework` with slices for device (`arm64`) and simulator
> (`arm64`, `x86_64`). The binary is large (~25 MB of embedded sign + voice
> data), same as the Android `.so`.

## Permissions

The SDK does **not** request location itself — your app obtains GPS and feeds it
into `onLocation`. Add the usage descriptions your app needs to `Info.plist`:

```xml
<!-- Required: your app collects GPS while navigating. -->
<key>NSLocationWhenInUseUsageDescription</key>
<string>We use your location to warn about speed limits and signs along your route.</string>
```

If you keep alerting while the app is backgrounded (a navigation app usually
does), also request *Always* authorization and enable the **Location updates**
background mode:

```xml
<key>NSLocationAlwaysAndWhenInUseUsageDescription</key>
<string>We keep warning about speed limits while navigation runs in the background.</string>

<key>UIBackgroundModes</key>
<array>
    <string>location</string>
    <!-- add "audio" too if you rely on the built-in voice player in the background -->
    <string>audio</string>
</array>
```

The SDK reaches the Vietmap service over the network to fetch segment data — no
extra iOS entitlement is needed for that.

## Quick Integration — `AlertViewManager`

`AlertViewManager` is the only public entry point. It owns the native engine and
serialises all GPS / network work on an internal serial `DispatchQueue` — the
public API is safe to call from any thread, and all callbacks arrive on the
**main thread**.

### 1. Create and configure

```swift
import AlertViewSDK

let manager = AlertViewManager()

manager.configure(
    baseUrl:       "https://driving.map.zone",   // Vietmap service base URL
    apiKeyId:      "<your-api-key-id>",           // issued by Vietmap
    apiKey:        "<your-api-key>",              // issued by Vietmap
    vehicleId:     "<your-vehicle-id>",           // issued by Vietmap
    vehicleType:   1,                             // Int    — 1..9 (see Vehicle Types)
    seats:         0,                             // Int    — seats for coaches; 0 = default for the type
    weights:       0,                             // Double — gross weight (tonnes) for trucks; 0 = default
    maxSnapMeters: 0)                             // Double — max snap distance (m); <= 0 = server default (~25 m)
```

> **Note:** Treat `apiKey` as a credential — do not log it, embed it in a public
> repo, or expose it in user-facing UI.
>

**Optional configuration** (call after `configure`, before `start`; all settings persist across reroutes):

| Method | Purpose |
|---|---|
| `setSegmentUrl(_ url: String)` | Override the segment-fetch endpoint. Empty = derive from `baseUrl`; when set, requests go to **exactly** that URL. Use this to route through your own proxy, then forward to `https://driving.map.zone/api/v2/tracking/network/packed`. The auth fetch always uses `baseUrl` and cannot be overridden. |
| `setExtraHeaders(_:) -> String?` | **Add** headers to the segment request. Returns `nil` on success, or an error message. Cannot override reserved headers (e.g. `Content-Type`, case-insensitive); a header key may not contain `:`. |
| `setExtraBodyFields(_:) -> String?` | **Add** body fields (string values) to the segment request. Returns `nil` on success. Cannot override the SDK's own fields. |
| `setMutedAlertTypes(_:)` | Mute specific alert voices (camera, toll, …). Speed-limit & speeding voices can never be muted. Call with no arguments to re-enable all. |
| `setVoiceMode(_: VoiceMode)` | Spoken sentences (`.full`, default) or a short chime (`.ding`). Applies on top of `setMutedAlertTypes`. See [Voice Mode & Voice Speed](#voice-mode--voice-speed). |
| `setVoiceSpeed(_: Float)` | Playback speed of the **built-in** voice player. `1.0` = recorded speed; clamped to `0.5`–`2.0`. |

`setExtraHeaders` / `setExtraBodyFields` reject empty keys, reserved keys, a
colon in a header key, and any value containing a newline; on rejection they
return a non-nil message and **keep the previous extras**. Pass an empty dict to
clear.

```swift
// Mute camera + toll voices; keep the visual signs.
manager.setMutedAlertTypes(.speedCamera, .toll)

// Add an extra header (returns nil on success).
if let err = manager.setExtraHeaders(["X-Trace-Id": traceId]) {
    print("rejected: \(err)")
}
```

### 2. Register callbacks

All callbacks fire on the main thread. Wire only the ones you need.

> **iOS difference:** the images are `UIImage?`, and — unlike Android — the
> **voice WAV is not part of the bitmap callback**. Voice is handled by the
> built-in player, or by `setVoiceCallback` if you take over playback.

```swift
// (a) Per-tick images — the core driver of your UI.
manager.setBitmapCallback { current, speedStatus,
                            next, nextDistMeters,
                            camera, cameraDistMeters,
                            toll, tollDistMeters in

    // current       : speed-sign for the current segment (nil = no link under the GPS point)
    // speedStatus   : 0 = within limit, 1 = approaching (within 5 km/h), 2 = speeding
    // next          : preview of the next speed sign, nextDistMeters ahead
    // camera        : preview of an upcoming speed camera, cameraDistMeters ahead
    // toll          : preview of an upcoming toll booth, tollDistMeters ahead
    self.ivSpeedSign.image = current
    self.statusBar.backgroundColor = [.systemGreen, .systemOrange, .systemRed][speedStatus]
    self.bindMiniSign(self.ivNext,   next,   nextDistMeters)
    self.bindMiniSign(self.ivCamera, camera, cameraDistMeters)
    self.bindMiniSign(self.ivToll,   toll,   tollDistMeters)
}

// (b) Per-tick restriction signs — optional, independent of (a).
//     Leave this unregistered and the SDK does not even build these images.
manager.setRestrictionCallback { stop, stopDistMeters,
                                 closed, closedDistMeters,
                                 vehicle, vehicleDistMeters,
                                 bua, buaDistMeters, inBua,
                                 turn, turnDistMeters in

    // Each slot is independent — a stretch of road can be inside a built-up
    // area, closed to your vehicle AND under a no-stopping order at once.
    // A nil image means "nothing for this slot"; hide it.
    self.bindMiniSign(self.ivStop,    stop,    stopDistMeters)
    self.bindMiniSign(self.ivClosed,  closed,  closedDistMeters)
    self.bindMiniSign(self.ivVehicle, vehicle, vehicleDistMeters)
    self.bindMiniSign(self.ivBua,     bua,     buaDistMeters)
    self.bindMiniSign(self.ivTurn,    turn,    turnDistMeters)
}

// (c) Fetch outcome — surfaces server errors. errorCode == 0 means success.
manager.setResultCallback { success, errorCode, message in
    if !success && errorCode != 0 {
        print("fetch error code=\(errorCode): \(message)")
    }
}

// (d) Route lost — the driver left the route you passed to start().
//     Wire this if your app can re-route; see "Reroute" below.
manager.setRerouteCallback { [weak self] lat, lng in
    self?.requestNewRoute(lat: lat, lng: lng)
}
```

> **Note (voice):** The iOS SDK has a **built-in, priority-aware `AVAudioPlayer`
> queue** that plays voice cues automatically — you don't have to do anything.
>
> Register a `VoiceCallback` only if you need to take over playback (mixing,
> ducking, gating on app state). Once you register it, the built-in player stops
> and you own all playback:
>
> ```swift
> manager.setVoiceCallback { wav, trigger, priority in
>     // wav      : WAV bytes (Data, PCM16 mono 22050 Hz)
>     // trigger  : native VoiceTrigger raw value (see Voice Alert Types)
>     // priority : 0 = "current" (shed first), 1 = normal (shed second),
>     //            2 = speeding (never shed)
>     myPlayer.enqueue(wav, priority: priority)
> }
> // Pass nil to restore the built-in player.
> ```
>
> WAV format: PCM 16-bit little-endian, mono, 22 050 Hz — ready for
> `AVAudioPlayer` or `AVAudioEngine`.
>
> In `.ding` mode your callback receives the chime clips too: `trigger`
> `22` = ding (camera, cấm dừng / cấm đỗ), `23` = ding over speed. While a
> callback is registered, `setVoiceSpeed` has no effect — set the playback
> speed on your own player instead (`enableRate` + `rate` on `AVAudioPlayer`).

### 3. Start a route & feed GPS

You start with the **full route polyline** (start→end), then feed every GPS frame.

```swift
// Start (or reroute — just call start() again with a new polyline):
manager.start(routePolyline)   // route geometry, encoded 1e6

// In your CLLocationManagerDelegate:
func locationManager(_ m: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
    guard let loc = locations.last else { return }
    manager.onLocation(
        lat:           loc.coordinate.latitude,
        lng:           loc.coordinate.longitude,
        bearing:       loc.course,                  // degrees, 0 = N, 90 = E
        speedKmh:      max(0, loc.speed) * 3.6,     // CLLocation.speed is m/s → km/h
        accuracy:      loc.horizontalAccuracy,      // horizontal accuracy in metres
        fixTimeMillis: Int64(loc.timestamp.timeIntervalSince1970 * 1000))
}

// When navigation ends (or before a reroute):
manager.reset()
```

> **Note:** `onLocation` takes speed in **km/h**. `CLLocation.speed` is in **m/s**
> (and `-1` when unknown), so clamp with `max(0, …)` and multiply by `3.6`.
> `fixTimeMillis` is the fix time in **milliseconds since the epoch (UTC)** — take
> it from `loc.timestamp`, not from a monotonic/uptime clock. Feed each fix as it
> arrives, at the usual GPS cadence (~1 Hz). Pass *snapped-to-route* coordinates
> if your app already runs map-matched navigation.

### 4. Reroute — when the driver leaves the route

The SDK alerts along the route you handed to `start()`; it does **not** compute
routes. If the driver takes a different road and the SDK cannot get back onto
that route, it reports the position once and then has nothing left to announce
until you hand it a new route.

```
   on route ──────────► driver takes a different road
                                   │
                                   ▼
                        RerouteCallback(lat, lng)    ← once per detour
                                   │
              your nav layer builds a new route from (lat, lng)
                                   │
                                   ▼
                        manager.start(newPolyline)   ← alerts resume
```

```swift
manager.setRerouteCallback { lat, lng in
    // Ask your directions provider for a fresh route from here, then:
    //   manager.start(newRoute.polyline1e6)
}
```

Registering a `RerouteCallback` is optional but strongly recommended for
navigation apps: without it, alerting simply stays quiet after a detour. It fires
at most once per detour and re-arms once the vehicle is back on a route.

### 5. Reset

```swift
manager.reset()    // releases route data + voice state
```

Call this when navigation finishes, or before a reroute if you want to free
engine memory immediately.

## Restriction Signs

`setRestrictionCallback` delivers five extra sign slots per GPS tick. They are
kept out of `setBitmapCallback` because they answer a different question and can
all apply at once — one stretch of road may sit inside a built-up area, be closed
to your vehicle **and** carry a no-stopping order.

| Slot | Shows | When |
|---|---|---|
| `stop` | Cấm đỗ / cấm dừng | Always, like the other slots. The voice is only spoken while the vehicle is slowing or stopped (≤ 18 km/h) — the order is about stopping |
| `closed` | Đường đang đóng | As soon as it is within the lookahead, so there is room to route around it |
| `vehicle` | Cấm phương tiện của bạn | Same; keyed to the `vehicleType` you passed to `configure` |
| `bua` | Khu dân cư (entry plate) or hết khu dân cư (end plate) | Entry plate for the whole time you are inside; end plate for ~5 s after leaving. **Sign only — this slot never speaks** |
| `turn` | Cấm rẽ / chỉ được rẽ … | When a turn restriction is ahead on the route |


## Voice Mode & Voice Speed

Both are host preferences: safe to change mid-navigation, and they persist
across reroutes and `reset()`. Neither changes what is shown on screen — every
sign is drawn exactly the same in either mode, at any speed.

```swift
manager.setVoiceMode(.ding)     // chimes instead of sentences
manager.setVoiceMode(.full)     // back to spoken alerts (default)

manager.setVoiceSpeed(1.5)      // 50% faster, pitch kept
manager.setVoiceSpeed(1.0)      // recorded speed (default)
```

### Voice mode

```
                        FULL (default)                  DING
cameras (all 4 kinds)   "phía trước có camera …"   →    chime
cấm đỗ / cấm dừng       "phía trước có biển …"     →    chime
over the speed limit    "bạn đang vượt quá …"      →    over-speed chime (distinct sound)
current / next speed limit · toll · built-up area ·
no overtaking · rest station · road closed ·
vehicle restricted                                 →    silent
```

- **Muting applies first.** A category muted with `setMutedAlertTypes` stays
  silent in `.ding` mode too (and a muted camera / toll still hides its sign).
- `.ding` is the only way to silence the speed-limit announcements, which
  `setMutedAlertTypes` deliberately cannot mute.
- Road closed and vehicle restricted stay silent in `.ding` mode because a
  chime cannot say "turn around" — rely on the sign in the restriction callback.
- The cấm dừng / cấm đỗ chime keeps the same rule as its sentence: only while the
  vehicle is slowing or stopped (≤ 18 km/h).
- An alert silenced by `.ding` mode does not hold back the alerts after it:
  a built-up-area plate nobody hears never delays the camera behind it.

### Voice speed

- `1.0` is the recorded speed (default); `1.5` plays 50% faster. Pitch is kept.
- Values are clamped to `0.5`–`2.0`; `.nan` restores `1.0`.
- Applies from the next clip — the clip already playing finishes at its speed.
- The built-in queue drops low-priority clips when more than ~6 s of audio is
  waiting. That limit is measured in real playback time, so a faster speed lets
  more clips through before any are dropped.
- Only affects the SDK's built-in player. With a `VoiceCallback` registered you
  own playback and its speed.

## Voice Alert Types (mute control)

Pass the categories you want to **mute** to `setMutedAlertTypes(...)`; everything
not listed keeps announcing. Speed-limit and speeding warnings are core safety
cues and can never be muted.

Muting a **camera or toll** category hides its icon as well as its voice. Muting
a **restriction** category (`.noParking`, `.noStopping`, `.roadClosed`,
`.vehicleRestricted`, `.buildupArea*`) silences the announcement but leaves the
sign on screen.

```swift
// Mute camera + toll voices; keep every other voice and all visual signs.
manager.setMutedAlertTypes(.speedCamera, .toll)

// Re-enable all voices:
manager.setMutedAlertTypes()
```

| `VoiceAlertType` | Meaning |
|---|---|
| `.speedCamera` | Speed camera |
| `.toll` | Toll booth |
| `.trafficEnforcementCamera` | Traffic-enforcement camera |
| `.redLightCamera` | Red-light camera |
| `.aiCamera` | AI camera |
| `.noLeftTurn` / `.noRightTurn` / `.noUTurn` / `.noStraight` | Turn restrictions — accepted, but currently silent: turn signs are shown without a spoken cue |
| `.noOvertaking` / `.noOvertakingEnd` | No-overtaking zone start / end |
| `.noParking` | Cấm đỗ — no parking |
| `.noStopping` | Cấm dừng — no stopping (the stricter of the two) |
| `.roadClosed` | Đường đang đóng — road closed ahead |
| `.vehicleRestricted` | Cấm phương tiện của bạn — your vehicle type is banned ahead |
| `.buildupAreaStart` / `.buildupAreaEnd` | Built-up (urban) area start / end — fires for a roadside sign, not for the built-up-area slot, which is silent |
| `.restStation` | Rest station |

## Vehicle Types

`configure(...)` takes a raw integer `vehicleType` in **1..9**.

| `vehicleType` | Description |
|---|---|
| `1` | Car (xe ô tô) |
| `2` | Motorcycle (xe mô tô) |
| `3` | Truck (xe tải) |
| `4` | Coach (xe khách) |
| `5` | Bus (xe bus) |
| `6` | Taxi (xe taxi) |
| `7` | Bicycle (xe đạp) |
| `8` | Pedestrian (người đi bộ) |
| `9` | Emergency (xe ưu tiên) |

## Error Codes (`ResultCallback`)

The codes below are the ones integrations hit in practice. `message` is a short,
display-ready string you can surface in your UI as-is.

| Code | Meaning |
|---|---|
| `0` | Success — segment data loaded |
| `1001` | A route point is outside the supported area (Vietnam) |
| `1002` | Missing points / invalid request |
| `1003` | Route geometry could not be read |
| `2001` | Device clock is more than 10 s out of sync with the server |
| `2002` | Unauthorized — `apiKeyId` / `apiKey` / bundle id mismatch |
| `2000`–`2999` (other) | Session could not be established — surfaced as "Your session has expired" |
| `3003` | Vehicle type not supported (must be `1..9`) |
| `3004` | Route did not map-match to any road link |
| `5000`–`5999` | Server temporarily unavailable — retry later |
| negative | Local SDK failure (no connectivity, or route data unavailable) — read `message` |

## Threading Model

```
 Any thread ──► start / onLocation / reset
                          │
                          ▼
              Internal serial DispatchQueue
              (HTTP fetch + 1-D snap + matching + image/voice render)
                          │
                          ▼
              Main thread ◄── BitmapCallback / ResultCallback / VoiceCallback
```

`start` and `onLocation` are serialised on the same queue, so they never race.
Callbacks always arrive on the main thread — update your UI directly.

## Demo

Check the demo app on [GitHub](https://github.com/mapzone-global/mapzone-alert-view-app-ios).
