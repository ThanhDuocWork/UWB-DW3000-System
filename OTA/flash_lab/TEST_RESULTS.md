# Boot swap flash lab: test results

This file records observed results, not a production bootloader qualification. The lab swaps dummy A/B data in the `swap_lab` partition; the ESP32 still boots the factory application at `0x10000`. No UWB self-test or real firmware-image execution is covered here.

## Host tests

| Test | Result | What it established |
| --- | --- | --- |
| `boot_swap_power_loss` | PASS | 4,420 simulated cut positions across request, forward swap, reverse swap and confirm. |
| `boot_swap_scratch_pause` | PASS | Restart from a partially programmed scratch sector preserves the journal and completes swap or rollback. |
| `boot_swap_journal_pause` | PASS | Eight simulated cases: request/checkpoint, both journal sectors and both A/B orientations; an unsealed record is ignored. |

The journal test was run with MSVC C++17 `/W4 /WX`; it printed `TC09: 8 journal pause cases ... passed`. Host results do not prove physical power-loss behavior.

## Board tests

The following are observations from the supplied serial logs. `A/B` means primary A, secondary B. A result marked PASS applies only to the observed checkpoint, not to every possible cut during a flash operation.

| Case | Result | Observed outcome |
| --- | --- | --- |
| TC01: reset in confirmed Idle | PASS | Idle B/A remained Idle B/A. |
| TC02: remove power in confirmed Idle | PASS | POWERON_RESET; Idle B/A remained Idle B/A. |
| TC03: normal swap | PASS | Reached Trial A/B. |
| TC04: reset unconfirmed Trial | PASS | Returned to Idle B/A. |
| TC05: remove power in unconfirmed Trial | PASS | Returned to Idle B/A. |
| TC06: cut after scratch 256/4096 | PASS | `swap_pause` stopped after the first scratch chunk; power-on restarted the scratch copy and reached Trial A/B. This was between chunk writes, not within one write. |
| TC07: cut during secondary copy | PASS at observed point | Recovery restarted the destination sector and reached Trial A/B; the exact final written byte at power loss was not established. |
| TC08: cut during primary copy | PASS at observed point | Recovery restarted the destination sector and reached Trial A/B; the exact final written byte at power loss was not established. |
| TC09a: journal request body before seal | PASS | From Idle A/B, `journal_pause request` wrote 28 bytes to sector 8; readback seal was `0xffffffff`. After POWERON_RESET the state was Idle A/B, `boot_ok=1`. The unsealed request was ignored. |
| TC09b: journal swap checkpoint body before seal | PASS | `journal_pause checkpoint` wrote 28 bytes to sector 7; readback seal was `0xffffffff`. After POWERON_RESET, recovery reached Trial B/A with `boot_ok=1`. A subsequent `reset` rolled back to Idle A/B with `boot_ok=1`. |
| TC10: cut during rollback | PASS at observed point | Recovery completed rollback to Idle B/A. |
| TC11: request swap while Trial | PASS | Command was refused; Trial A/B remained unchanged. |
| TC12: confirm then reset | PASS | Confirmed Idle A/B; reset preserved Idle A/B. |

The last supplied board log ended in **Idle primary=A secondary=B boot_ok=1** after TC09b and its rollback. `ClearCommError` on USB removal was monitor disconnection, not a swap error.

## TC09b evidence and limits

1. Started from Idle A/B; `journal_pause checkpoint` committed the request, copied secondary sector 3 to scratch sector 6, then stopped after the journal sector 7 body write (`written=28/32`, `seal=0xffffffff`).
2. Power-on recovery (`reset_reason=1`) repeated the scratch copy and completed the swap: `state=Trial primary=B secondary=A boot_ok=1`.
3. Without confirmation, `reset` caused software reset (`reset_reason=3`) and rollback: `state=Idle primary=A secondary=B boot_ok=1`.

The first startup Trial line was captured, so the later rollback did not hide it. Neither TC09a nor TC09b exercises power loss in the middle of a single flash write.
