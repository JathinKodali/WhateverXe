# Experimental GU603 branch

This development branch preserves the existing local GU603VV graphics work. It is based on upstream WhateverXe tag `0.7.0` (commit `d0b7fb6`), not the latest upstream code. The pre-publication comparison found 429 identical upstream files, nine modified files and ten added files, with no upstream files missing. Added publication documentation and packaged regression tests are separate from that original comparison.

## Scope

The captured target is an ASUS GU603VV with i7-13620H, Intel PCI identity 8086:A7A8 revision 4, and a specific captured GPU resource profile. This is not general support for every GU603, A7A8 GPU or macOS build.

The source preserves experiments for resource admission, configure wake scope, unavailable media-engine/PAVP guards, internal-panel routing, panel watermark/clock policy, eDP register handling and backlight control. Hardware, driver-byte and profile guards must be preserved. Some resource-admission changes are profile-gated rather than boot-argument-gated: do not assume the entire patch set is disabled simply by removing experimental boot arguments.

## Build and tests

Follow the upstream README for SDK and Lilu prerequisites, then use:

```bash
python3 tools/test.py
python3 tools/build.py
```

The CPU runner includes packaged GU603 C++ policy tests and a watermark-clock replay. To also check the backlight fallback path in a fresh process after the test build:

```bash
build/tests/gu603_backlight_tests fallback
```

Unit tests and compilation do not establish successful boot, sleep/wake, brightness, media decoding or graphics acceleration on hardware. The expanded packaged suite passed all 212 tests during publication preparation on 17 November 2026. The RELEASE build of WhateverXe and AppleIntelTGLCompat and the Compat-version check also passed, using MacKernelSDK snapshot 05094e5 and the local Lilu developer bundle. The separate backlight fallback test passed. Compiler warnings remain. Hardware validation is still pending.

## Installation status

No bootable EFI or stable binary release is supplied by this branch. Keep a working boot configuration and test changes on a separate USB. Select dependencies and macOS versions using the upstream documentation and exact driver guards in the source; do not assume that newer drivers are compatible with the captured guards. The separate EFI project currently provides only an incomplete sanitized configuration example.

Upstream authorship, history and license are retained. These local experiments do not imply upstream endorsement.
