# Validation and storage boundaries

Changed Block Tracking SDK 0.1.0 targets Windows 10 version 2004/build 19041+ x64 and Windows 11 x64. This evaluation
accepts only the independently identified, unpartitioned 64 MiB Titan Storage
Fault Lab synthetic scratch disk. Physical, system and other virtual disks are
inventory-only. It is an upper disk filter, not a file-system minifilter.

## Offline checks

Normal and AddressSanitizer CTest builds pass 4,001,753 assertions across
10,000 randomized bitmaps compared with an independent bitset, clipped final
blocks, coalesced ranges and malformed state/inventory refusal. Twenty additional
installer checks reject wrong identities, paths, directives and unsigned
catalogs. These assertions do not represent millions of disk workloads: offline
tests open no driver and write no raw disk. Applications, SDK and both setups
compile W4/WX, static CRT, CFG, ASLR and NX.

## Completed guest campaigns

The frozen candidate completed both a normal campaign and a targeted Driver
Verifier campaign in the owned Hyper-V Windows build 19045
(10.0.19045) guest. Each completed installation, its explicit runtime
fixtures, package removal and required normal restarts. The guest was restored
to its original checkpoint and finished Off; host boot/security policy was not
changed. These results cover that candidate and fixture, not every Windows PC.

New kernel candidates used guest-only test signing. Independent fixture drivers
retained their verified Microsoft signatures. This is functional qualification,
not a Microsoft attestation/HLK/WHQL claim. Returned Microsoft packages and the
final embedded setup still require their own trust, membership and installation
checks before installable binaries are distributed.

## Demo and activation qualification

The production runtime is configured for one hour per product per Windows boot.
The production functional campaigns **did not wait a full hour**. A separately
pinned private build of the same runtime code used an explicit twelve-second
duration to exercise real expiry, per-product independence, no reset by closing
and reopening handles or repeated Begin, full-mode activation after expiry, and
full-mode persistence beyond another twelve seconds. Products 8 and 10 expired
on their own deadlines and were not unlocked by product 7's activation. Product
9 was already full after its own USB
test and is not described as an unused slot in the lifecycle probe.

The USB short-runtime fixture independently kept a read pending across the real
cutoff, observed completion after expiry and before its independent physical-read
timeout budget, and refused a new export after expiry. Genuine full-mode transfer
recovery followed. Known-IOCTL malformed buffers, restricted callers, limited
handle access, wrong-handle/cross-product proofs, modified signatures, replaced
and consumed nonces, and accepted-proof replay were refused. Five original
broker grants were independently verified against the public P256 key. These
tests are not proof of unbreakable licensing or immunity to kernel compromise.

Normal and AddressSanitizer offline builds also passed the strict shared runtime
installer's 115 identity/trust-refusal checks. License-server HTTP and signature
verification run during activation under a monotonic whole-operation budget.
Data paths retain their local entitlement checks and normal processing costs;
encrypted transports still perform their own TLS work.

## Storage fixture scope

Both guest campaigns passed known synthetic-disk writes/readback and exact
changed ranges, immutable frozen maps, continued tracking, direct-kernel stale
ACK/map refusal across owners, actual pending/busy checkpoint observation,
quiescent boundaries, requests across Stop/Start, unknown-path invalidation,
genuine demo/full use and restoration of the explicit scratch-byte ranges.
Administrative and restricted insufficient-access control opens were refused at
CREATE in synchronous and overlapped modes, with successful legitimate-owner
recovery after every refusal. The private per-device association wrapper also
restored exact original UpperFilters presence/type/raw bytes and a healthy node.
The four bounded writers touched only the separately captured 64 KiB range at
6 MiB. The viewer itself never generates disk writes. These are integrity and
lifecycle checks, not physical storage throughput or latency benchmarks.

The frozen CBT driver input SHA256 is 4ce1c39c8107818f0f9b6c9d0d002d16bd9410a167aa73e0475f1479668c501c.
The shared production runtime input SHA256 is 5ee4e64afeffe527a02b74fc50bdcc7eed4808e6e3ec9a3db028039057c6bf80.

Physical storage stacks, Windows 11, Secure Boot/HVCI, boot/paging/crash paths,
power loss, sleep/resume, low resources, prolonged workloads and other filters
require separate qualification before attachment scope is expanded. No VSS
requester, automatic baseline, image backup or restore engine is bundled.

## Data integrity contract

Normal disk reads and writes are forwarded unchanged. The filter records
conservative ranges; it does not provide encryption, ransomware protection,
write blocking, failure injection or backup-file storage.

Requested writes are recorded before forwarding, including failed and partial
requests. Checkpoint is refused while any potentially mutating request remains
pending; this avoids declaring a boundary before earlier writes resolve. Pending
requests are counted even when submitted before tracking began, and that count
is not reset by Stop/Start. A completion that reaches the current active interval
marks its affected ranges, regardless of the request's earlier tracking owner.
An unknown mutator completion invalidates that interval. Duplicate conservative
ranges are acceptable; omitted acknowledged changes are not. Generation numbers
are monotonic across intervals and owners, preventing a stale map read or
acknowledgment from applying to a replacement baseline.

Unknown potentially mutating control paths invalidate history rather than claim
complete coverage. Invalid, expired or absent history requires a new baseline.
Demo expiry never blocks disk traffic. Full activation does not reconstruct
expired or otherwise missing write history.

The application requires explicit attachment, preserves unrelated per-device
UpperFilters, refuses physical/system/partitioned targets and does not issue a
disk restart. Uninstall first detaches owned associations, requests a restart if
needed, and only then removes the unused owned package. The shared runtime
remains installed for other products.

## Primary implementation references

- [Microsoft storage filter architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/storage-filter-drivers)
- [Device-specific upper-filter installation](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/installing-a-filter-driver)
- [Write request semantics](https://learn.microsoft.com/en-us/previous-versions/windows/drivers/ifs/irp-mj-write)
- [Volume Shadow Copy Service, required separately for a coordinated backup integration](https://learn.microsoft.com/en-us/windows/win32/vss/volume-shadow-copy-service-portal)
