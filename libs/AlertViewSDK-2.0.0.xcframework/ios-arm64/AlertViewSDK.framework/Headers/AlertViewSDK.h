//
//  AlertViewSDK.h
//  AlertViewSDK
//
//  Umbrella header for the AlertViewSDK framework module. Exposing the
//  Obj-C++ bridge here lets the framework's own Swift sources (AlertViewBridge,
//  AlertViewManager) call into the native route engine without a bridging
//  header (frameworks can't use one).
//

#import <Foundation/Foundation.h>

//! Project version number for AlertViewSDK.
FOUNDATION_EXPORT double AlertViewSDKVersionNumber;

//! Project version string for AlertViewSDK.
FOUNDATION_EXPORT const unsigned char AlertViewSDKVersionString[];

#import <AlertViewSDK/AVRouteBridge.h>
