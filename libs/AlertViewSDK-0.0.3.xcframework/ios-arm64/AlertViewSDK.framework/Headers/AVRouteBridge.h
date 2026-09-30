//
//  AVRouteBridge.h
//  AlertViewSDK
//
//  Obj-C++ bridge to the ROUTE-based Alert View engine
//  (vietmap::route::RouteAlertSession). This is the SDK's only native unit and
//  the iOS analogue of Android's alert_view_jni.cpp.
//
//  The implementation (.mm) #includes the header-only C++ route engine and
//  owns the single RouteAlertSession instance, the synchronous NSURLSession
//  HTTP transport, bitmap generation (speed_signs::generate*Bmp) and voice
//  generation (voice::generate*Voice).
//
//  Swift never touches C++ — it talks to this Obj-C class only. Most apps use
//  the higher-level AlertViewManager (km/h→m/s, serial queue, UIImage decode,
//  built-in voice player) rather than this bridge directly.
//

#ifndef AVRouteBridge_h
#define AVRouteBridge_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Speeding status reported alongside each bitmap tick.
/// 0 = within limit, 1 = approaching (within 5 km/h), 2 = exceeding.
typedef NS_ENUM(NSInteger, AVSpeedStatus) {
    AVSpeedStatusSafe = 0,
    AVSpeedStatusNearLimit = 1,
    AVSpeedStatusOverLimit = 2,
};

/// Per-tick speed-sign bitmaps (BMP bytes, 32-bit). Any slot may be nil.
typedef void (^AVBitmapBlock)(NSData *_Nullable currentBmp,
                              AVSpeedStatus speedStatus,
                              NSData *_Nullable nextBmp, int nextDistMeters,
                              NSData *_Nullable cameraBmp, int cameraDistMeters,
                              NSData *_Nullable tollBmp, int tollDistMeters);

/// One voice clip (WAV PCM16 mono 22050Hz) with its native VoiceTrigger value
/// and VoicePriority (0 = current/"hiện tại", 1 = normal, 2 = speeding).
typedef void (^AVVoiceBlock)(NSData *wav, int trigger, int priority);

/// Per-tick raw + snapped GPS for a debug map overlay.
typedef void (^AVDebugBlock)(double rawLat, double rawLng,
                             double snapLat, double snapLng,
                             BOOL snapValid, long long matchedLinkId);

/// Per-segment fetch outcome. code: 0 ok, >0 server envelope code,
/// <0 local SDK error (see message for detail).
typedef void (^AVResultBlock)(BOOL success, int errorCode, NSString *message);

// Exported explicitly: Release hides symbols by default; this class is the
// public ObjC surface and must stay linkable from host apps.
__attribute__((visibility("default")))
@interface AVRouteBridge : NSObject

+ (instancetype)sharedInstance;

// ---- Configuration ----
- (void)configureBaseUrl:(NSString *)baseUrl
               apiKeyId:(NSString *)apiKeyId
                  apiKey:(NSString *)apiKey
                bundleId:(NSString *)bundleId
               vehicleId:(NSString *)vehicleId
             vehicleType:(int)vehicleType
                   seats:(int)seats
                 weights:(double)weights
           maxSnapMeters:(double)maxSnapMeters
    NS_SWIFT_NAME(configure(baseUrl:apiKeyId:apiKey:bundleId:vehicleId:vehicleType:seats:weights:maxSnapMeters:));

/// Override the segment-fetch URL only. Optional; call after configure.
/// Empty = derive from the base URL; non-empty = use EXACTLY this URL
/// (no subpath). The auth fetch always stays on the base URL.
- (void)setSegmentUrl:(NSString *)url NS_SWIFT_NAME(setSegmentUrl(_:));

/// Add EXTRA request headers to every segment-fetch POST. The SDK's own
/// reserved headers cannot be overridden: a reserved key (case-insensitive),
/// an empty key, or a value containing a newline is rejected and the previous
/// extras are kept. Pass an empty dict to clear. Persists across routes; never
/// affects the auth fetch.
/// Returns nil on success, otherwise a human-readable error message.
- (nullable NSString *)setExtraHeaders:(NSDictionary<NSString *, NSString *> *)headers
    NS_SWIFT_NAME(setExtraHeaders(_:));

/// Add EXTRA fields to every segment-fetch request body as JSON string values.
/// The existing fields (vehicleId, vehicleType, seats, weights, maxSnapMeters,
/// pointsPolyline) cannot be overridden; reserved/empty keys or newline values
/// are rejected, keeping the previous extras. Pass an empty dict to clear.
/// Returns nil on success, otherwise a human-readable error message.
- (nullable NSString *)setExtraBodyFields:(NSDictionary<NSString *, NSString *> *)fields
    NS_SWIFT_NAME(setExtraBodyFields(_:));

// ---- Lifecycle ----
/// Set (or replace, on reroute) the full route polyline (encoded 1e6).
/// Splits into ≤5km segments and fetches segment 0. Returns NO on failure.
- (BOOL)setRoute:(NSString *)polyline;

/// Feed one GPS tick (speed already in m/s; timestamp = epoch milliseconds).
/// Fires bitmap/voice/debug blocks. Expected cadence ~1 Hz — faster fixes are
/// dropped and the previous frame is kept.
- (void)processGpsLat:(double)lat
                  lng:(double)lng
              bearing:(double)bearing
              speedMs:(double)speedMs
             accuracy:(double)accuracy
            timestamp:(long long)timestamp
    NS_SWIFT_NAME(processGps(lat:lng:bearing:speedMs:accuracy:timestamp:));

/// Navigation finished — release route data + voice debounce state.
- (void)reset;

/// Host voice-mute mask (bit i = VoiceTrigger value i). Persists across routes.
- (void)setMutedVoiceTriggers:(uint64_t)mask;

- (int)segmentCount;
- (int)currentSegment;

// ---- Callbacks ----
- (void)setBitmapBlock:(nullable AVBitmapBlock)block;
- (void)setVoiceBlock:(nullable AVVoiceBlock)block;
- (void)setDebugBlock:(nullable AVDebugBlock)block;
- (void)setResultBlock:(nullable AVResultBlock)block;

@end

NS_ASSUME_NONNULL_END

#endif /* AVRouteBridge_h */
