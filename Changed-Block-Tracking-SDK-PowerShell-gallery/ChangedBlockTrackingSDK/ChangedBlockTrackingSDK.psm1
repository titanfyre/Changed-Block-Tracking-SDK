$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'lib/ChangedBlockTrackingSDKJsonSDKPowerShell.dll')
function Connect-ChangedBlockTrackingSDKSDK { <# .SYNOPSIS Opens an inert product SDK session. #> [CmdletBinding()]param() [TitanCSA.Products.ChangedBlockTrackingSDK.PowerShell.Client]::new() }
function Invoke-ChangedBlockTrackingSDKSDK { <# .SYNOPSIS Calls a documented product operation with an argument object. #> [CmdletBinding()]param([Parameter(Mandatory)]$Client,[Parameter(Mandatory)][string]$Operation,[hashtable]$Arguments=@{}) $Client.Call($Operation,($Arguments|ConvertTo-Json -Depth 100 -Compress)) | ConvertFrom-Json -Depth 100 }
function Disconnect-ChangedBlockTrackingSDKSDK { <# .SYNOPSIS Closes the SDK session and owned resources. #> [CmdletBinding()]param([Parameter(Mandatory)]$Client) $Client.Dispose() }
Export-ModuleMember -Function Connect-ChangedBlockTrackingSDKSDK,Invoke-ChangedBlockTrackingSDKSDK,Disconnect-ChangedBlockTrackingSDKSDK
