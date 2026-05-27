# NVME_CTRL_MARGINAL Patch Series Analysis

## Branch: `fpin_v11d` (based on `branch_v7.0`)

This document describes each patch in the `fpin_v11d` branch. The series
implements NVMe marginal path handling driven by FC Fabric Performance
Impact Notifications (FPIN). When an FC switch detects link degradation
it sends an FPIN; these patches propagate that signal through the SCSI FC
transport into the NVMe multipath layer so that I/O is steered away from
degraded paths.

The branch contains 13 commits. Patches 1-11 form the feature series;
patches 12-13 are local development aids marked "do not commit".

---

## Patch 1 — `fc_els: use 'union fc_tlv_desc'`

- **Commit:** a74ee22104f7
- **Author:** Hannes Reinecke / John Meneghini
- **Files:** `include/uapi/scsi/fc/fc_els.h`, `drivers/scsi/lpfc/lpfc_els.c`,
  `drivers/scsi/lpfc/lpfc_hw4.h`, `drivers/scsi/scsi_transport_fc.c`

### What it does

Refactors the FC ELS TLV descriptor type system to eliminate unsafe pointer
casts. The old code used a generic `struct fc_tlv_desc` with an opaque
`desc_value[]` payload and cast it to specific descriptor types
(`fc_fn_li_desc`, `fc_diag_lnkflt_desc`, etc.) throughout the driver and
transport code.

This patch:

1. **Renames** `struct fc_tlv_desc` to `struct fc_tlv_desc_hdr` — it now
   serves only as the tag+length header.
2. **Introduces** `union fc_tlv_desc` which overlays all known descriptor
   types (`li`, `deli`, `peer_congn`, `congn`, `fpin_reg`, `lnkflt`,
   `cg_sig`) along with the header, giving type-safe access via named
   members (e.g., `tlv->li`, `tlv->lnkflt`) instead of casts.
3. **Updates** the macros `FC_TLV_DESC_HDR_SZ`, `FC_TLV_DESC_LENGTH_FROM_SZ`,
   and `FC_TLV_DESC_SZ_FROM_LENGTH` to work with the new header struct and
   access `tlv->hdr.desc_len` instead of the old `tlv->desc_len`.
4. **Updates** `fc_tlv_next_desc()` to accept and return `union fc_tlv_desc *`
   with proper byte-level pointer arithmetic.
5. **Rewrites** the flexible-array containers (`fc_els_fpin`, `fc_els_edc`,
   `fc_els_rdf`, `fc_els_rdf_resp`, `fc_els_edc_resp`) to use
   `union fc_tlv_desc desc[]` instead of `struct fc_tlv_desc desc[]`.
6. **Resolves a conflict** with commit 44b6169ada7f by moving the
   `fc_els_rdf_hdr` and `fc_els_rdf_resp_hdr` `__struct_group()` definitions
   out of the UAPI header `fc_els.h` and into `lpfc_hw4.h` as local
   anonymous structs, since the `__struct_group()` helpers are incompatible
   with the new union-based approach.
7. **Eliminates all `(struct fc_fn_li_desc *)tlv` style casts** in
   `lpfc_els.c` and `scsi_transport_fc.c` — functions like
   `lpfc_els_rcv_fpin_li()`, `fc_fpin_li_stats_update()`, etc. now take the
   concrete descriptor type directly (e.g., `struct fc_fn_li_desc *li`)
   instead of a generic `struct fc_tlv_desc *tlv`.

### Why it matters

This is a prerequisite for the rest of the series. The later patches
(especially patch 7) need to walk FPIN TLV descriptors and access
`fc_fn_li_desc` fields like `pname_list` — the union makes that access
type-safe and avoids compiler warnings about strict-aliasing violations
and field-spanning writes.

---

## Patch 2 — `nvme: add NVME_CTRL_MARGINAL flag`

- **Commit:** eae988d9b594
- **Author:** Bryan Gurney
- **Files:** `drivers/nvme/host/nvme.h`, `drivers/nvme/host/core.c`

### What it does

Adds the `NVME_CTRL_MARGINAL` flag (bit 7) to the `enum nvme_ctrl_flags`
enumeration and provides a helper function to test it:

```c
NVME_CTRL_MARGINAL = 7,

static inline bool nvme_ctrl_is_marginal(struct nvme_ctrl *ctrl)
{
    return test_bit(NVME_CTRL_MARGINAL, &ctrl->flags);
}
```

The flag is cleared during controller initialization in `nvme_init_ctrl()`.

### Why it matters

This defines the core abstraction that the rest of the series builds on.
All NVMe subsystem code can now query whether a controller is on a degraded
path by calling `nvme_ctrl_is_marginal()`.

---

## Patch 3 — `nvme-fc: marginal path handling`

- **Commit:** 973f35d59a6d
- **Author:** Hannes Reinecke
- **Files:** `drivers/nvme/host/fc.c`, `drivers/nvme/host/multipath.c`

### What it does

Integrates the NVME_CTRL_MARGINAL flag into the NVMe multipath path
selection algorithms and the FC error recovery path:

**In `multipath.c`:**

1. **`__nvme_find_path()` (NUMA policy):** When evaluating
   `NVME_ANA_OPTIMIZED` paths, marginal controllers are skipped — they
   fall through to the `NVME_ANA_NONOPTIMIZED` case instead of being
   selected as the optimized path. This means marginal-but-optimized paths
   are treated equivalently to non-optimized paths for path selection.

2. **`nvme_round_robin_path()`:** An optimized path that is marginal no
   longer short-circuits the search with `goto out`. Instead the loop
   continues looking for a non-marginal optimized path; a marginal optimized
   path is kept only as a fallback.

3. **`nvme_path_is_optimized()`:** Now returns `false` for marginal
   controllers regardless of ANA state. This affects all callers that check
   whether an existing path is still optimal.

**In `fc.c`:**

4. **`nvme_fc_ctrl_connectivity_loss()`:** Clears the NVME_CTRL_MARGINAL
   flag before initiating a controller reset. This ensures that after
   hardware replacement (which triggers a connectivity loss / reset cycle)
   the controller starts clean.

### Why it matters

This is where the marginal flag actually affects I/O routing. Without this
patch, setting the flag would have no effect on path selection.

---

## Patch 4 — `nvme: sysfs: emit the marginal path state in show_state()`

- **Commit:** 6603754da4a0
- **Author:** Bryan Gurney / John Meneghini
- **Files:** `drivers/nvme/host/sysfs.c`

### What it does

Modifies `nvme_sysfs_show_state()` to display `"marginal"` instead of the
normal state name (typically `"live"`) when the `NVME_CTRL_MARGINAL` flag
is set on a controller.

Reading `/sys/class/nvme/nvmeX/state` will show:
- `"marginal"` — if the controller is flagged
- `"live"`, `"connecting"`, etc. — normal state names otherwise

### Why it matters

Provides user-space visibility into which controllers are currently
considered marginal. This is essential for monitoring, alerting, and manual
debugging of multipath configurations.

---

## Patch 5 — `nvme-multipath: queue-depth support for marginal paths`

- **Commit:** 68eaa2c4fa49
- **Author:** John Meneghini
- **Files:** `drivers/nvme/host/multipath.c`

### What it does

Extends the queue-depth I/O policy (`nvme_queue_depth_path()`) to handle
marginal paths:

1. **Skips marginal controllers** in the main selection loop by adding a
   `continue` when `nvme_ctrl_is_marginal(ns->ctrl)` is true. This prevents
   marginal paths from being chosen for load-balanced I/O.

2. **Adds a fallback path** for when no optimized or non-optimized path is
   found (all paths are marginal). In that case, it falls back to
   `__nvme_find_path(head, numa_node_id())`, which will select the best
   available marginal path rather than returning no path at all.

### Why it matters

Patch 3 handled the NUMA and round-robin policies but not queue-depth.
This completes the coverage so that all three multipath I/O policies
correctly deprioritize marginal paths while still maintaining a fallback to
avoid I/O failure when all paths are marginal.

---

## Patch 6 — `nvme-fc: add nvme_fc_modify_rport_fpin_state()`

- **Commit:** e22092d4492c
- **Author:** John Meneghini / Hannes Reinecke
- **Files:** `drivers/nvme/host/fc.c`, `include/linux/nvme-fc-driver.h`

### What it does

Adds the external API for the SCSI FC transport layer to set or clear the
marginal flag on NVMe-FC controllers. Implements three new functions:

1. **`nvme_fc_lport_from_wwpn(u64 wwpn)`** — Looks up an NVMe-FC local
   port by its WWPN. Iterates `nvme_fc_lport_list` under `nvme_fc_lock`,
   takes a reference, and returns the matching lport (or NULL).

2. **`nvme_fc_rport_from_wwpn(lport, u64 rport_wwpn)`** — Looks up an
   NVMe-FC remote port on a given lport by WWPN. Only matches ports that
   have `FC_PORT_ROLE_NVME_TARGET` set.

3. **`nvme_fc_fpin_set_state(lport, u64 wwpn, bool marginal)`** — Finds
   the rport matching `wwpn`, then iterates all controllers on that rport
   under `rport->lock` and sets or clears `NVME_CTRL_MARGINAL` on each.

4. **`nvme_fc_modify_rport_fpin_state(u64 local_wwpn, u64 remote_wwpn,
   bool marginal)`** — The exported entry point (`EXPORT_SYMBOL_GPL`).
   Resolves the lport from `local_wwpn`, then delegates to
   `nvme_fc_fpin_set_state()`.

The function prototype is also added to `include/linux/nvme-fc-driver.h`.

### Why it matters

This bridges the SCSI FC transport world and the NVMe-FC world. The SCSI
transport knows about FPINs and FC rports; the NVMe-FC layer knows about
NVMe controllers. This function translates "rport X is marginal" into
"set NVME_CTRL_MARGINAL on all controllers attached via rport X".

---

## Patch 7 — `scsi: scsi_transport_fc: add fc_host_fpin_set_nvme_rport_marginal()`

- **Commit:** b387fd396a61
- **Author:** John Meneghini / Hannes Reinecke
- **Files:** `drivers/scsi/scsi_transport_fc.c`, `include/scsi/scsi_transport_fc.h`

### What it does

Adds `fc_host_fpin_set_nvme_rport_marginal()` to the SCSI FC transport
layer. This function:

1. **Parses the FPIN payload** by walking the TLV descriptor list, looking
   specifically for `ELS_DTAG_LNK_INTEGRITY` descriptors.

2. **For each Link Integrity descriptor**, extracts the `attached_wwpn` and
   iterates over the `pname_list` (the list of port names affected by the
   link integrity event).

3. **For each affected WWPN** (excluding the `attached_wwpn` itself):
   - Finds the corresponding FC rport via `fc_find_rport_by_wwpn()`.
   - Under `shost->host_lock`, checks that the rport is
     `FC_PORTSTATE_ONLINE` and has `FC_PORT_ROLE_NVME_TARGET`.
   - If so, sets the rport state to `FC_PORTSTATE_MARGINAL`.
   - Calls `nvme_fc_modify_rport_fpin_state()` (guarded by
     `IS_ENABLED(CONFIG_NVME_FC)`) to propagate the marginal state to NVMe
     controllers.

The function is exported via `EXPORT_SYMBOL` and declared in
`scsi_transport_fc.h`. The `nvme_fc_modify_rport_fpin_state()` prototype is
forward-declared in `scsi_transport_fc.c` under `#if IS_ENABLED(CONFIG_NVME_FC)`.

### Why it matters

This is the key glue between FPIN reception and NVMe marginal state. It
takes the raw FPIN payload, identifies which NVMe target ports are affected,
and triggers the marginal state change in both the FC transport (rport
state) and the NVMe subsystem (controller flag).

---

## Patch 8 — `scsi: lpfc: enable FPIN notification for NVMe`

- **Commit:** 630303c5b52d
- **Author:** John Meneghini / Hannes Reinecke
- **Files:** `drivers/scsi/lpfc/lpfc_els.c`

### What it does

Hooks the lpfc (Emulex/Broadcom) driver's FPIN receive path into the NVMe
marginal state machinery. In `lpfc_els_rcv_fpin()`, after the existing call
to `fc_host_fpin_rcv()` (which forwards the FPIN to user space), this patch
adds a conditional call to `fc_host_fpin_set_nvme_rport_marginal()`:

```c
if (deliver) {
    fc_host_fpin_rcv(shost, fpin_length, (char *)fpin, 0);
    if (vport->cfg_enable_fc4_type & LPFC_ENABLE_NVME) {
        fc_host_fpin_set_nvme_rport_marginal(shost,
            fpin_length, (char *)fpin);
    }
}
```

The call is gated on the vport having NVMe enabled (`LPFC_ENABLE_NVME`).

### Why it matters

This is the lpfc driver-specific integration point. Without this, lpfc
would receive FPINs and forward them to user space but never trigger NVMe
marginal path handling.

---

## Patch 9 — `scsi: qla2xxx: enable FPIN notification for NVMe`

- **Commit:** 1fa6874c3208
- **Author:** John Meneghini / Hannes Reinecke
- **Files:** `drivers/scsi/qla2xxx/qla_isr.c`

### What it does

Adds a single line to `qla27xx_process_purex_fpin()` in the qla2xxx
(QLogic/Marvell) driver, calling `fc_host_fpin_set_nvme_rport_marginal()`
immediately after `fc_host_fpin_rcv()`:

```c
fc_host_fpin_rcv(vha->host, pkt_size, (char *)pkt, 0);
fc_host_fpin_set_nvme_rport_marginal(vha->host, pkt_size, (char *)pkt);
```

Unlike lpfc, qla2xxx does not gate on NVMe enablement — the
`fc_host_fpin_set_nvme_rport_marginal()` function itself checks for
`FC_PORT_ROLE_NVME_TARGET` on each rport.

### Why it matters

This is the qla2xxx driver-specific integration point, analogous to
patch 8 for lpfc. Together patches 8 and 9 ensure both major FC HBA
vendors' drivers propagate FPIN events to the NVMe marginal path logic.

---

## Patch 10 — `scsi: scsi_transport_fc: user support for clearing NVME_CTRL_MARGINAL`

- **Commit:** 4a13198938ad
- **Author:** John Meneghini
- **Files:** `drivers/scsi/scsi_transport_fc.c`

### What it does

Refactors `fc_rport_set_marginal_state()` — the sysfs handler for writing
to `/sys/class/fc_remote_ports/rport-X:Y-Z/port_state` — to:

1. **Add SMP safety:** All reads and writes of `rport->port_state` are now
   performed under `shost->host_lock` (using `spin_lock_irqsave` /
   `spin_unlock_irqrestore`). The original code had no locking.

2. **Propagate to NVMe:** When transitioning `Online -> Marginal` or
   `Marginal -> Online`, calls `nvme_fc_modify_rport_fpin_state()` with
   `marginal=true` or `marginal=false` respectively (guarded by
   `IS_ENABLED(CONFIG_NVME_FC)`).

3. **Restructures as a switch statement** for clarity, replacing the
   nested if-else chain.

This enables user-space administrators to manually control the marginal
state:

```bash
# Mark a port as marginal
echo "Marginal" > /sys/class/fc_remote_ports/rport-13:0-5/port_state

# Clear marginal state
echo "Online" > /sys/class/fc_remote_ports/rport-13:0-5/port_state
```

### Why it matters

Provides manual override capability for testing and operational recovery.
An administrator can clear a marginal state without waiting for a
controller reset, or can manually mark a port as marginal for testing
or maintenance purposes.

---

## Patch 11 — `scsi: qla2xxx: Fix 2 memcpy field-spanning write issue`

- **Commit:** bb2078829f4a
- **Author:** Gustavo A. R. Silva / Chris Leech
- **Files:** `drivers/scsi/qla2xxx/qla_def.h`, `drivers/scsi/qla2xxx/qla_isr.c`,
  `drivers/scsi/qla2xxx/qla_nvme.c`, `drivers/scsi/qla2xxx/qla_os.c`

### What it does

Fixes kernel `memcpy` field-spanning write warnings that appeared during
FPIN testing. The warnings looked like:

```
kernel: memcpy: detected field-spanning write (size 60) of single
field "((uint8_t *)fpin_pkt + buffer_copy_offset)"
at drivers/scsi/qla2xxx/qla_isr.c:1221 (size 44)
```

The fix involves restructuring `struct purex_item`:

1. **Converts `iocb` to a true flexible array:** Replaces the nested struct
   containing a fixed `uint8_t iocb[64]` with a bare `uint8_t iocb[]
   __counted_by(size)`. This gives the compiler correct bounds information.

2. **Moves `default_item` to the end of `scsi_qla_host`:** Since
   `struct purex_item` now ends with a flexible array member, the embedded
   `default_item` in `scsi_qla_host` must be at the end of the struct.
   Uses the `TRAILING_OVERLAP()` macro to create a union between the
   flexible array `default_item.iocb` and a fixed-size
   `__default_item_iocb[QLA_DEFAULT_PAYLOAD_SIZE]` array.

3. **Adjusts allocation in `qla24xx_alloc_purex_item()`:** Uses
   `struct_size(item, iocb, size)` instead of the old manual size
   calculation, properly accounting for the flexible array.

4. **Fixes `qla24xx_free_purex_item()`:** When freeing the default item,
   zeroes both the `__default_item_iocb` overlay and the `default_item`
   struct itself, and orders them to avoid a null pointer dereference (the
   `default_item` struct is zeroed last).

5. **Updates `qla_nvme.c`:** Changes `item->iocb.iocb[3]` to
   `item->iocb[3]` since the nested struct wrapper is removed.

### Why it matters

This is a bug fix discovered during FPIN testing. The field-spanning write
warnings indicate that `memcpy` was writing beyond the declared bounds of
the `iocb` field, which could cause undefined behavior under
`CONFIG_FORTIFY_SOURCE`. The fix makes the data structure correctly
represent the actual allocation pattern.

### Known issue — compile error

This patch introduces a compile error in `drivers/scsi/qla2xxx/qla_isr.c`.
The struct change converts `item->iocb` from a nested struct (containing
`uint8_t iocb[64]`) to a bare flexible array (`uint8_t iocb[]`), but two
functions were not updated to match:

```
drivers/scsi/qla2xxx/qla_isr.c: In function 'qla27xx_copy_multiple_pkt':
drivers/scsi/qla2xxx/qla_isr.c:882:44: error: request for member 'iocb' in something not a structure or union
  882 |         if (total_bytes > sizeof(item->iocb.iocb))
      |                                            ^
drivers/scsi/qla2xxx/qla_isr.c:883:48: error: request for member 'iocb' in something not a structure or union
  883 |                 total_bytes = sizeof(item->iocb.iocb);
      |                                                ^
drivers/scsi/qla2xxx/qla_isr.c: In function 'qla27xx_copy_fpin_pkt':
drivers/scsi/qla2xxx/qla_isr.c:1170:44: error: request for member 'iocb' in something not a structure or union
 1170 |         if (total_bytes > sizeof(item->iocb.iocb))
      |                                            ^
drivers/scsi/qla2xxx/qla_isr.c:1171:48: error: request for member 'iocb' in something not a structure or union
 1171 |                 total_bytes = sizeof(item->iocb.iocb);
      |                                                ^
```

The patch updated some `item->iocb.iocb` references (in `qla24xx_copy_std_pkt`
and `qla2xxx_process_purls_iocb`) but missed four occurrences in
`qla27xx_copy_multiple_pkt()` (lines 882-883) and `qla27xx_copy_fpin_pkt()`
(lines 1170-1171). These still use the old nested-struct accessor
`item->iocb.iocb`, which no longer exists after the flex-array conversion.

The fix is to replace `sizeof(item->iocb.iocb)` with
`QLA_DEFAULT_PAYLOAD_SIZE` in these four locations, matching the intent of
the original code (clamping the copy size to the default buffer capacity).

---

## Patch 12 — `scsi: lpfc: makefile change` (DO NOT COMMIT)

- **Commit:** a8d98cf7f359
- **Author:** John Meneghini
- **Files:** `drivers/scsi/lpfc/Makefile`

### What it does

Adds `-Wflex-array-member-not-at-end` to the compiler warnings enabled
under `WARNINGS_BECOME_ERRORS` in the lpfc Makefile.

### Why it matters

Local development/testing aid to catch flex-array positioning issues at
compile time. Marked "do not commit" — not intended for upstream
submission.

---

## Patch 13 — `NVME_CTRL_MARGINAL_Analysis.md` (DO NOT COMMIT)

- **Commit:** 370e213d31de
- **Author:** John Meneghini
- **Files:** `NVME_CTRL_MARGINAL_Analysis.md`

### What it does

Adds this analysis document to the repository.

### Why it matters

Documentation/tracking artifact. Marked "do not commit" — not intended for
upstream submission.

---

## Patch Series Architecture Summary

The series implements a complete end-to-end pipeline for NVMe marginal
path management:

```
FC Switch detects link degradation
        |
        v
FC Switch sends FPIN (Link Integrity event)
        |
        v
FC HBA receives FPIN
        |
        +--- lpfc: lpfc_els_rcv_fpin()       [Patch 8]
        +--- qla2xxx: qla27xx_process_purex_fpin()  [Patch 9]
        |
        v
fc_host_fpin_set_nvme_rport_marginal()        [Patch 7]
  - Parses FPIN LI TLV descriptors            [uses Patch 1 union types]
  - Sets FC rport state to FC_PORTSTATE_MARGINAL
  - Calls nvme_fc_modify_rport_fpin_state()
        |
        v
nvme_fc_modify_rport_fpin_state()             [Patch 6]
  - Looks up NVMe-FC lport/rport by WWPN
  - Sets NVME_CTRL_MARGINAL on all controllers [Patch 2]
        |
        v
NVMe multipath path selection                [Patches 3, 5]
  - NUMA policy: marginal optimized -> treated as non-optimized
  - Round-robin: marginal skipped if alternatives exist
  - Queue-depth: marginal excluded from load balancing
  - All policies: fallback to marginal if no other paths
        |
        v
User visibility via sysfs                    [Patch 4]
  - /sys/class/nvme/nvmeX/state -> "marginal"

Manual control via sysfs                      [Patch 10]
  - echo "Marginal" > /sys/class/fc_remote_ports/rport-X:Y-Z/port_state
  - echo "Online" > /sys/class/fc_remote_ports/rport-X:Y-Z/port_state

Recovery paths:
  - Controller reset clears NVME_CTRL_MARGINAL  [Patches 2, 3]
  - Manual sysfs write to "Online"              [Patch 10]
```

### Key Design Decisions

1. **Marginal != dead:** Marginal paths are deprioritized but not disabled.
   If all paths are marginal, I/O still flows through the best available
   marginal path.

2. **Reset clears marginal:** The assumption is that a controller reset
   implies hardware replacement or recovery, so the marginal state should
   be cleared.

3. **NVMe-only targeting:** `fc_host_fpin_set_nvme_rport_marginal()` only
   affects rports with `FC_PORT_ROLE_NVME_TARGET`, leaving SCSI target
   rports untouched (SCSI already has its own marginal path infrastructure
   via dm-multipath).

4. **User override:** Administrators can manually set or clear marginal
   state via the FC rport sysfs interface, which propagates through to the
   NVMe layer.
