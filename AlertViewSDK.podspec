Pod::Spec.new do |s|
  s.name          = "MapZoneAlertView"
  s.version       = "0.0.3"
  s.summary       = "MapZone Alert View SDK – real-time speed alert for iOS."
  s.description   = <<-DESC
    MapZoneAlertViewSDK provides speed alert functionality with a
    shared C++ core. Features include real-time speed limit detection, camera alerts,
    toll booth detection, route progress tracking, and TTS announcements.
  DESC

  s.homepage     = "https://github.com/mapzone-global/mapzone-alert-view-ios"
  s.license      = { :type => "MIT", :file => "LICENSE" }
  s.author       = { "MapZone" => "support@mapzone.vn" }

  s.platform              = :ios, "12.0"
  s.ios.deployment_target = "12.0"
  s.swift_version         = "5.0"

  s.source        = { :git => "https://github.com/mapzone-global/mapzone-alert-view-ios.git", :tag => "ios-#{s.version}" }

  s.vendored_frameworks = "MapZoneAlertView.xcframework"
  s.frameworks    = "Foundation", "AVFoundation"
  s.libraries     = "c++"
  s.pod_target_xcconfig = { "EXCLUDED_ARCHS[sdk=iphonesimulator*]" => "i386" }
end
