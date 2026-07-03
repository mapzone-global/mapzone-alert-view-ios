Pod::Spec.new do |s|
  s.name          = "AlertViewSDK"
  s.version       = "0.0.2"
  s.summary       = "MapZone Alert View SDK – real-time speed alert for iOS."
  s.description   = <<-DESC
    MapZoneAlertViewSDK provides GPS tracking and speed alert functionality with a
    shared C++ core. Features include real-time speed limit detection, camera alerts,
    toll booth detection, route progress tracking, and TTS announcements.
  DESC

  s.homepage     = "https://maps.vietmap.vn"
  s.license      = { :type => "MIT", :file => "LICENSE" }
  s.author       = { "MapZone" => "support@mapzone.vn" }

  s.platform              = :ios, "12.0"
  s.ios.deployment_target = "12.0"
  s.swift_version         = "5.0"

  # TODO: điền URL git nội bộ trước khi push spec.
  s.source        = { :git => "https://github.com/mapzone-global/mapzone-alert-view-ios.git", :tag => "ios-#{s.version}" }

  s.vendored_frameworks = "AlertViewSDK.xcframework"
  s.frameworks    = "Foundation", "AVFoundation"
  s.libraries     = "c++"
  s.pod_target_xcconfig = { "EXCLUDED_ARCHS[sdk=iphonesimulator*]" => "i386" }
end
