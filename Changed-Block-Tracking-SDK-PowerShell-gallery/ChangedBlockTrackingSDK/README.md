# Changed-Block-Tracking-SDK PowerShell SDK

PowerShell7.4+ Windows x64. `Install-Module ChangedBlockTrackingSDK -Scope CurrentUser`.

```powershell
$client=Connect-ChangedBlockTrackingSDKSDK
try { Invoke-ChangedBlockTrackingSDKSDK -Client $client -Operation status } finally { Disconnect-ChangedBlockTrackingSDKSDK -Client $client }
```

Use -Arguments with a hashtable matching [schema](docs/API.md). Native errors retain their Windows code through the inner exception. Matching driver installed separately.
