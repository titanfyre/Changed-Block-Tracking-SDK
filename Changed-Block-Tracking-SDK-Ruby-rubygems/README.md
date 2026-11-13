# Changed-Block-Tracking-SDK Ruby SDK

`gem install changed-block-tracking-sdk`

Windows x64 Ruby3.1+.

```ruby
require "changed_block_tracking_sdk"
ChangedBlockTrackingSDKSDK::Client.open { |client| p client.call("status") }
```

[API and operation schema](docs/API.md). Matching driver installed separately.
