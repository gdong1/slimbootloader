# Re-signing `SlimBootloader.bin` with a New Key Set

> **Scope.** This document explains how to replace *every* SBL-owned signing
> key inside an already-built `SlimBootloader.bin` **without rebuilding SBL
> from source**, using only tools that already ship in the SBL tree.
>
> **Assumption.** The image is a normal, redundant SBL layout — both
> `TS0` (primary) and `TS1` (backup) partitions are present. All commands
> below therefore treat the two partitions symmetrically.
>
> **Non-goals.** This procedure does not change the signing algorithm, the
> key size, the hash type, the signing scheme, or the on-flash HashStore
> slot count. Those are fixed at build time and cannot be altered
> post-build.

---

## 0. Automated path (recommended)

For the standard case, use
[SblOpen/BootloaderCorePkg/Tools/ResignSlimBootloader.py](../BootloaderCorePkg/Tools/ResignSlimBootloader.py),
which automates §5–§9 in a single command. Once you have the `Tools/`
folder set up per §4:

```powershell
# Dry-run: discover layout, validate keys, print plan, write no bytes.
python Tools\ResignSlimBootloader.py `
    -i .\SlimBootloader.bin `
    -o .\SlimBootloader_signed.bin `
    -k .\NewSblKeys `
    --verify -v `
    --audit-log .\resign_audit.txt

# Actual re-sign.
python Tools\ResignSlimBootloader.py `
    -i .\SlimBootloader.bin `
    -o .\SlimBootloader_signed.bin `
    -k .\NewSblKeys `
    --audit-log .\resign_audit.txt
```

The tool always recomputes the Stage1A `PUBKEY_MASTER` slot from the
supplied Master key and writes it back. If the Master key is unchanged
the patch is a no-op; if it was rotated, this is the step that completes
the rotation (see §9 for the manual-procedure equivalent).

To turn the resigned `SlimBootloader.bin` into a shipping IFWI, re-run
your platform's normal stitch flow (`StitchIfwi.py`), which invokes
`BtgSign.py` as part of its own pipeline. Boot Guard signing is out of
scope for this tool.

Follow the manual procedure below only when:

- the automated tool fails or does not support your board's layout,
- you need to audit exactly what will be changed before running it,
- your program's release process requires a documented, step-by-step
  procedure independent of any tool.

---

## 1. Audience

- Signing / release engineers who receive a stitched `SlimBootloader.bin`
  from a build team and must re-sign it with production keys.
- Security teams performing periodic key rotation on shipping images
  without a full source rebuild.
- Anyone operating in an air-gapped signing enclave where the SBL build
  tree is not available.

## 2. Trust chain

```
Boot Guard OEM root  --signs--> KeyManifest (KM)
KM                   --signs--> Boot Policy Manifest (BPM)
BPM                  --measures--> Stage1A  (IBB)
Stage1A              --contains--> HashStoreTable  (signature "_HS_")
                                     |
                                     |-- PUBKEY_MASTER  hash  (anchor for KEYH)
                                     |-- STAGE1B       hash   (content only)
                                     |-- STAGE2        hash   (content only)
                                     |-- PAYLOAD       hash   (content only)
                                     +-- PAYLOAD_FWU   hash   (content only)
Master private key   --signs--> KEYH container
KEYH content         --contains hashes of--> CFG_DATA / FWU / OS / CONT_DEF pub keys
CFG_DATA key         --signs--> external CFGDATA blob
FWU key              --signs--> Capsule / FirmwareUpdate.bin
Container key        --signs--> Boot container header
Container-comp key   --signs--> Individual container components
OS1 / OS2 keys       --signs / verifies--> OS verified-boot images
```

The only bytes that live *inside signed code regions* are:
1. `PUBKEY_MASTER` hash in Stage1A's `HashStoreTable` (both TS0 and TS1).
2. The OEM public-key hash fused into the SoC (for Boot Guard).

Everything else (KEYH, CFGDATA, containers, OS images, capsules) is a
re-signable container that lives at a known offset in the flash image.

## 3. Two cases: Master unchanged vs Master rotated

Every SBL key can be replaced independently, but the **Master key**
gets special treatment because its public-key hash is baked into
Stage1A's `HashStoreTable`. That leads to two practical cases:

| Case | Master key | Stage1A bytes touched | Downstream keys |
|---|---|---|---|
| **Master unchanged** | reused from original build | none (or a same-value rewrite that is a no-op) | new |
| **Master rotated**   | new | `HashStoreTable` `PUBKEY_MASTER` slot patched in TS0 and TS1 | new |

Keeping the Master key is preferred when possible — no observable
Stage1A bytes change, so the image is minimally disturbed. Rotating the
Master is required if the Master key itself is compromised or must
expire.

The automated tool ([§0](#0-automated-path-recommended)) handles both
cases uniformly: it always recomputes the Stage1A `PUBKEY_MASTER` slot
from the supplied Master key, which is a no-op when the key is
unchanged and completes the rotation when it is not.

## 4. Prerequisites

- Python 3.8+
- OpenSSL 1.1.1 or 3.x on `PATH`, or `OPENSSL_PATH` set to its directory.
- A copy of the SBL Tools scripts (`SblOpen/BootloaderCorePkg/Tools/`).
  You do **not** need the full SBL source tree — just copy that one
  directory next to your work folder. No build required.
- Write access to a working directory.
- The input `SlimBootloader.bin` you intend to rotate.
- **If you plan to keep the same Master key:** the *existing*
  `MasterTestKey_Priv_RSA*.pem` from the original build. You cannot
  rotate downstream keys without it, because KEYH must be re-signed
  with the same Master key that Stage1A already trusts.

### Environment

Layout for a stand-alone signing folder (no SBL source tree needed):

```
C:\resign\work\
   Tools\               <- copied from SblOpen\BootloaderCorePkg\Tools
   NewSblKeys\          <- generated with GenerateKeys.py (see §6)
   SlimBootloader.bin   <- the image to re-sign
```

```powershell
# Windows PowerShell
$WORK = "C:\resign\work"
mkdir $WORK -Force | Out-Null

# One-time: drop the SBL Tools folder into $WORK\Tools
# (from any SBL checkout; only the .py files are used).
Copy-Item -Recurse <sbl-checkout>\SblOpen\BootloaderCorePkg\Tools `
          "$WORK\Tools"

# Every-run environment
$env:PYTHONPATH   = "$WORK\Tools"
$env:SBL_KEY_DIR  = "$WORK\NewSblKeys"     # created in step 6
$env:OPENSSL_PATH = "C:\Openssl\bin"       # only if openssl not on PATH
Set-Location $WORK
```

```bash
# Linux
WORK=~/resign/work && mkdir -p "$WORK"

# One-time: drop the SBL Tools folder into $WORK/Tools
cp -r <sbl-checkout>/SblOpen/BootloaderCorePkg/Tools "$WORK/Tools"

# Every-run environment
export PYTHONPATH="$WORK/Tools"
export SBL_KEY_DIR="$WORK/NewSblKeys"
export OPENSSL_PATH=/usr/bin                # only if openssl not on PATH
cd "$WORK"
```

All commands below assume these variables are set and that the current
working directory is `$WORK`. Wherever a manual step invokes
`python <tool>.py`, run it as `python Tools\<tool>.py` (Windows) or
`python Tools/<tool>.py` (Linux).

## 5. Inspect the input image

Before touching anything, capture the layout of the image so every
subsequent replace step targets the correct offset and every re-signed
container has the correct component list.

### 5.1 Dump the IFWI tree

```powershell
python -c "from IfwiUtility import IFWI_PARSER; \
d=open(r'$WORK\..\SlimBootloader.bin','rb').read(); \
IFWI_PARSER.print_tree(IFWI_PARSER.parse_ifwi_binary(bytearray(d)))"
```

Expected structure (names may vary slightly by platform, offsets/lengths
will differ):

```
IFWI
  BIOS
    TS0
      SG1A          <-- Stage1A_A.fd
      SG1B
      SG02          <-- Stage2 + payload / containers
      KEYH          <-- KEYHASH container (Master-signed)
      ...
    TS1
      SG1A          <-- Stage1A_B.fd (redundant copy)
      SG1B
      SG02
      KEYH
      ...
```

Record every leaf name and offset that appears under both `TS0` and `TS1`.
Any component whose name appears twice (once per partition) must be
updated **twice**, symmetrically.

### 5.2 Enumerate the current KEYH content

Extract the KEYH container from the input image and view it to learn
which downstream key usages it currently anchors:

```powershell
# Extract KEYH from TS0
python Tools\GenContainer.py extract `
    -i .\SlimBootloader.bin -n KEYH -od $WORK\keyh_ts0
python Tools\GenContainer.py view -i $WORK\keyh_ts0\KEYH.bin
```

The output lists every `HASH_USAGE` entry currently baked into KEYH
(e.g. `PUBKEY_CFG_DATA`, `PUBKEY_FWU`, `PUBKEY_OS`, `PUBKEY_CONT_DEF`,
optional `PUBKEY_OEM_*`). This is the exact list you will need to
regenerate in §7.

### 5.3 Enumerate every signed container in the image

Any leaf under `TS0` or `TS1` that is a container (e.g. `SG02`, `EPLD`,
`UEFI_VARIABLE` if wrapped, boot containers, etc.) must be examined for
its own signing keys. Extract and view each of them:

```powershell
python Tools\GenContainer.py extract `
    -i .\SlimBootloader.bin -n SG02 -od $WORK\sg02_ts0
python Tools\GenContainer.py view -i $WORK\sg02_ts0\<container>.bin
```

Note for each container:
- Its **auth type** (`RSA2048_PKCS1_SHA2_256`, `RSA3072_PSS_SHA2_384`,
  `SHA2_*`, or `NONE`). Containers with `auth = NONE` do **not** need
  re-signing; do not force-sign them.
- Its **header key** (used to sign the container header).
- Each component's **key file / KEY_ID** and **compression** (`lz4`,
  `lzma`, `dummy`).
- Each component's **SVN**.

Save this to a local inventory file — you will feed the same values back
when re-signing in §8. Losing the SVN silently downgrades the container
and may brick anti-rollback-enforced parts.

## 6. Generate the new key set

Use SBL's key generator so all filenames match the entries in
`SingleSign.SIGNING_KEY`. This is critical: the tool chain looks keys
up by these exact names.

```powershell
python Tools\GenerateKeys.py -k $env:SBL_KEY_DIR
```

This produces every `MasterTestKey_Priv_RSA{2048,3072}.pem`,
`ConfigTestKey_*`, `FirmwareUpdateTestKey_*`, `ContainerTestKey_*`,
`ContainerCompTestKey_*`, `OS1_TestKey_*`, `OS2_TestKey_*`.

To keep the same Master key, overwrite the freshly-generated Master
key files with the ones from the original build:

```powershell
Copy-Item C:\keys\OldSblKeys\MasterTestKey_Priv_RSA2048.pem $env:SBL_KEY_DIR -Force
Copy-Item C:\keys\OldSblKeys\MasterTestKey_Priv_RSA3072.pem $env:SBL_KEY_DIR -Force
```

For **production**: do not use the "test" filenames unchanged. Generate
production keys in your HSM / KMS, export the public halves and (only for
signing runs) the private halves in PEM form, and drop them in
`$SBL_KEY_DIR` under the *same file names* the `SIGNING_KEY` table
expects. Alternatively, rename them and update `SIGNING_KEY` in a fork
of `SingleSign.py`.

**Invariants to preserve when replacing keys** (also enforced by the
automated tool's `--verify` mode in §0):

- RSA modulus size must match the original (2048 stays 2048; 3072 stays
  3072). Mixing sizes will break KEYH slot sizes.
- Public exponent should stay `0x10001` (F4).
- Do not change hash type (SHA-256 vs SHA-384) or signing scheme
  (PKCS1 vs PSS). These are compiled into Stage1A/Stage1B's verifier.
  `adjust_hash_type()` derives the hash type from key size:
  `RSA2048 → SHA2_256`, `RSA3072 → SHA2_384`.

## 7. Regenerate the KEYH container

Author a KEYH layout file `$WORK/keyh_layout.txt` listing every downstream
usage discovered in §5.2. Example for a typical platform:

```python
# keyh_layout.txt -- Python-tuple syntax read by GenExtKeyHashStore.py
(HASH_USAGE['PUBKEY_CFG_DATA'], 'KEY_ID_CFGDATA_RSA3072'),
(HASH_USAGE['PUBKEY_FWU'],      'KEY_ID_FIRMWAREUPDATE_RSA3072'),
(HASH_USAGE['PUBKEY_CONT_DEF'], 'KEY_ID_CONTAINER_RSA3072'),
(HASH_USAGE['PUBKEY_OS'],       'KEY_ID_OS1_PUBLIC_RSA3072'),
```

Only include usages your image actually contains. Never add usages that
were not in the original KEYH — Stage1B checks the usage bitmap.

Sign the new KEYH with the Master key:

```powershell
python Tools\GenExtKeyHashStore.py `
    -l $WORK\keyh_layout.txt `
    -k KEY_ID_MASTER_RSA3072 `
    -a SHA2_384 -s RSA_PSS `
    -svn 0 `
    -o $WORK\KEYH.bin
```

Notes:
- `-k` must be the same Master Key ID/size that the image was originally
  built with, unless you are rotating the Master key (§9).
- `-a` / `-s` must match the image's build-time hash and signing scheme.
  If unsure, use `-a AUTO` and let the tool derive from the key size.
- `-svn` is the KEYH SVN. Read it from the *existing* KEYH via
  `GenContainer.py view` and pass the same or a higher number — never
  lower. Downgrading SVN will brick anti-rollback-enforced parts.

## 8. Splice new KEYH and re-signed artifacts into the image

The `SlimBootloader.bin` is an IFWI-style blob with named leaves. Use
the IFWI parser to overwrite the KEYH leaf in both partitions atomically.
The following one-liner works for any redundant SBL image without
requiring a `BoardConfig.py`:

```powershell
python - <<'PY'
import os
from IfwiUtility import IFWI_PARSER

work = os.environ['WORK'] if 'WORK' in os.environ else r'C:\resign\work'
img_in  = r'.\SlimBootloader.bin'
img_out = os.path.join(work, 'SlimBootloader_step8.bin')
keyh    = os.path.join(work, 'KEYH.bin')

data = bytearray(open(img_in, 'rb').read())
root = IFWI_PARSER.parse_ifwi_binary(data)

for path in ('IFWI/BIOS/TS0/KEYH', 'IFWI/BIOS/TS1/KEYH'):
    node = IFWI_PARSER.locate_component(root, path)
    if node is None:
        raise SystemExit("Leaf not found: %s" % path)
    new = open(keyh, 'rb').read()
    if len(new) > node.length:
        raise SystemExit("New KEYH (%d) exceeds region size (%d)" % (len(new), node.length))
    padded = new + b'\xff' * (node.length - len(new))
    data[node.offset:node.offset + node.length] = padded
    print("Patched %s @ 0x%08X (size 0x%X)" % (path, node.offset, node.length))

open(img_out, 'wb').write(data)
print("Wrote", img_out)
PY
```

The rest of §8/§9 use the same one-liner pattern for every artifact
replaced. After each replacement step, the *output* image of the previous
step becomes the *input* of the next.

### 8.1 Re-sign the CFGDATA blob

Extract the current signed CFGDATA blob, strip its signature, re-sign it
with the new `KEY_ID_CFGDATA_*`, and splice it back:

```powershell
# Extract CFGDATA from the current image
python Tools\CfgDataTool.py export `
    -i $WORK\SlimBootloader_step8.bin -b 0 -o $WORK\cfg_ts0
python Tools\CfgDataTool.py export `
    -i $WORK\SlimBootloader_step8.bin -b 1 -o $WORK\cfg_ts1

# Re-sign (both partitions typically carry identical CFGDATA)
python Tools\CfgDataTool.py sign `
    $WORK\cfg_ts0\CfgDataExt.bin `
    -k KEY_ID_CFGDATA_RSA3072 `
    -a AUTO -s RSA_PSS -svn <preserve_original_svn> `
    -o $WORK\CfgDataExt_signed.bin

# Splice back
python Tools\CfgDataStitch.py `
    -i $WORK\SlimBootloader_step8.bin `
    -o $WORK\SlimBootloader_step8b.bin `
    -c $WORK\CfgDataExt_signed.bin
```

Preserve the original CFGDATA SVN by reading it from the old blob header
(`CfgDataTool.py view -v 1 CfgDataExt.bin`).

### 8.2 Re-sign every container

For each container discovered in §5.3, use `GenContainer.py replace` for
every component. Because the container header itself is signed by the
container key, you must replace *every* component in place with its
new key mapping. Iterate over the component list captured in §5.3.

Example for a container named `SG02` with two components
(`OS1B`, `MOSS`):

```powershell
$img = "$WORK\SlimBootloader_step8b.bin"
$out = "$WORK\SlimBootloader_step8c.bin"

# Replace each component; header is re-signed automatically each time
python Tools\GenContainer.py replace `
    -i $img -o $out -n OS1B `
    -f <extracted_OS1B_uncompressed.bin> `
    -c lz4 `
    -k KEY_ID_CONTAINER_COMP_RSA3072 `
    -s <original_component_svn>

python Tools\GenContainer.py replace `
    -i $out -o $out -n MOSS `
    -f <extracted_MOSS_uncompressed.bin> `
    -c dummy `
    -k KEY_ID_CONTAINER_COMP_RSA3072 `
    -s <original_component_svn>
```

Notes:
- Extract each component first via `GenContainer.py extract`. Do **not**
  re-compress the extracted `.lz4`/`.lzma` blob yourself — pass the raw
  file and let `replace` recompress with the original algorithm.
- The container header key comes from the container's declared auth
  type; the tool signs it as part of `replace`.
- Preserve every component's original SVN. Read it from
  `GenContainer.py view`.
- Any container whose original auth type was `NONE` must not be re-signed
  — skip it.

### 8.3 EPAYLOAD (if present)

`EPAYLOAD` is just another container. If it exists (check §5.1 output)
and is non-empty (i.e., not all `0xFF`), apply the same procedure as
§8.2 with `-n EPLD` (or whatever the leaf is named) and the appropriate
component key IDs.

### 8.4 Capsule / firmware-update signing

The `FirmwareUpdate.bin` (or capsule) is *not* stored inside
`SlimBootloader.bin`; it is a separately-shipped artifact validated at
runtime against `PUBKEY_FWU` in KEYH. When you rotate the FWU key, you
must sign every future capsule with the new `KEY_ID_FIRMWAREUPDATE_*`.

Plan a **transitional capsule**: sign it with the *old* FWU key (so
already-fielded units accept it) but have it install an image containing
the *new* KEYH (which advertises the *new* FWU key). Skipping this step
strands fielded units without a rotation path.

## 9. Rotate the Master key (skip if unchanged)

If you regenerated the Master key in §6 (did not copy the old one back),
Stage1A no longer trusts KEYH. Patch the `PUBKEY_MASTER` slot in both
Stage1A copies:

```powershell
python - <<'PY'
import os, ctypes, hashlib
from IfwiUtility import IFWI_PARSER
from BuildUtility import HASH_USAGE, HashStoreTable, HashStoreData

work    = os.environ['WORK']
img_in  = os.path.join(work, 'SlimBootloader_step8c.bin')
img_out = os.path.join(work, 'SlimBootloader_step9.bin')

# 1. Compute new Master public-key hash.
#    Use openssl to export raw modulus||exponent, hashed with the build's SIGN_HASH.
#    (Here we use SHA-384 to match RSA-3072; use SHA-256 for RSA-2048.)
import subprocess, re
pub = subprocess.check_output([
    os.environ.get('OPENSSL_PATH','openssl').rstrip('\\/') + os.sep + 'openssl',
    'rsa', '-pubout', '-text', '-noout',
    '-in', os.path.join(os.environ['SBL_KEY_DIR'], 'MasterTestKey_Priv_RSA3072.pem'),
]).decode().replace('\r','').replace('\n','').replace('  ','')
m = re.search(r'modulus(.*)publicExponent:\s+(\d+)\s+', pub)
mod = bytes.fromhex(m.group(1).replace(':',''))
if mod[0] == 0 and (mod[1] & 0x80): mod = mod[1:]
exp = int(m.group(2)).to_bytes(4, 'big')
digest = hashlib.sha384(mod + exp).digest()   # or sha256 for RSA-2048

data = bytearray(open(img_in, 'rb').read())
root = IFWI_PARSER.parse_ifwi_binary(data)

# 2. Patch the PUBKEY_MASTER entry inside every Stage1A HashStoreTable.
SIG = HashStoreTable.HASH_STORE_SIGNATURE
for part in ('TS0', 'TS1'):
    node = IFWI_PARSER.locate_component(root, 'IFWI/BIOS/%s/SG1A' % part)
    if node is None:
        raise SystemExit("Stage1A not found for %s" % part)
    stage = data[node.offset:node.offset + node.length]
    off = stage.find(SIG)
    if off < 0:
        raise SystemExit("_HS_ table not found in %s Stage1A" % part)
    hs = HashStoreTable.from_buffer(stage, off)
    cur = off + ctypes.sizeof(HashStoreTable)
    end = off + hs.UsedLength
    patched = False
    while cur < end:
        e = HashStoreData.from_buffer(stage, cur)
        digest_off = cur + ctypes.sizeof(HashStoreData)
        digest_end = digest_off + e.DigestLen
        if e.Usage == HASH_USAGE['PUBKEY_MASTER']:
            if e.DigestLen != len(digest):
                raise SystemExit("digest length mismatch: %d vs %d" % (e.DigestLen, len(digest)))
            stage[digest_off:digest_end] = digest
            patched = True
            break
        cur = digest_end
    if not patched:
        raise SystemExit("PUBKEY_MASTER slot not present in %s HashStoreTable" % part)
    data[node.offset:node.offset + node.length] = stage
    print("Patched PUBKEY_MASTER in %s Stage1A" % part)

open(img_out, 'wb').write(data)
print("Wrote", img_out)
PY
```

Notes:
- The hash algorithm (`sha256` / `sha384`) **must** match the image's
  build-time `SIGN_HASH_TYPE`. Wrong hash length is caught by the size
  check above.
- No other byte inside Stage1A changes; the reset vector, entry point,
  and code image are untouched.
- Boot Guard, if enabled on the platform, re-measures Stage1A after
  this patch. Boot Guard KM/BPM re-signing is out of scope for this
  procedure — it is handled by your platform's normal stitch flow
  (`StitchIfwi.py`), which invokes `BtgSign.py` on the final image.

## 10. Rotation checklist

Copy this checklist into your release ticket and tick every box.

- [ ] Backup of original `SlimBootloader.bin` archived.
- [ ] Old key store archived separately (private keys under HSM control).
- [ ] New key store generated with `GenerateKeys.py`; production keys
      installed with correct filenames.
- [ ] Master key: **keeping / rotating** (circle one).
- [ ] If keeping: old Master key files copied into new `SBL_KEY_DIR`.
- [ ] Layout dump captured (§5.1); leaf inventory saved.
- [ ] KEYH content decoded (§5.2); usage list captured.
- [ ] Every signed container's key IDs, auth type, SVNs captured (§5.3).
- [ ] New KEYH generated with matching SVN or higher (§7).
- [ ] KEYH replaced in both TS0 and TS1 (§8).
- [ ] CFGDATA re-signed with original or higher SVN and re-stitched
      into both partitions (§8.1).
- [ ] Every signed container re-signed with correct component keys,
      compression, and SVNs, in both partitions (§8.2 / §8.3).
- [ ] If rotating Master: `PUBKEY_MASTER` slot patched in both Stage1A copies
      (§9), digest length matches original.
- [ ] Resigned image boots on the target platform (or under QEMU for
      QEMU builds); every `RSA verification` / `HASH verification`
      line in the serial log reports `Success`.
- [ ] Audit log attached to the release ticket (automated tool emits
      one via `--audit-log`; see §0). Confirm that only the leaves you
      intended to change appear in the "leaves that changed" list.
- [ ] Transitional capsule signed with old FWU key and containing new
      KEYH prepared for fielded units (if FWU key was rotated).
- [ ] Resigned image re-stitched through the platform's normal
      `StitchIfwi.py` flow (which invokes `BtgSign.py` on the final
      IFWI). Boot Guard signing is out of scope for this procedure.

## 11. Tool quick reference

| Purpose | Tool |
|---|---|
| Generate SBL key set | [SblOpen/BootloaderCorePkg/Tools/GenerateKeys.py](../BootloaderCorePkg/Tools/GenerateKeys.py) |
| Sign a raw file with a KEY_ID | [SblOpen/BootloaderCorePkg/Tools/SingleSign.py](../BootloaderCorePkg/Tools/SingleSign.py) |
| Build / re-build the KEYH container | [SblOpen/BootloaderCorePkg/Tools/GenExtKeyHashStore.py](../BootloaderCorePkg/Tools/GenExtKeyHashStore.py) |
| View / extract / replace / sign container components | [SblOpen/BootloaderCorePkg/Tools/GenContainer.py](../BootloaderCorePkg/Tools/GenContainer.py) |
| View / sign / stitch external CFGDATA | [SblOpen/BootloaderCorePkg/Tools/CfgDataTool.py](../BootloaderCorePkg/Tools/CfgDataTool.py), [SblOpen/BootloaderCorePkg/Tools/CfgDataStitch.py](../BootloaderCorePkg/Tools/CfgDataStitch.py) |
| Parse IFWI layout / locate leaves | [SblOpen/BootloaderCorePkg/Tools/IfwiUtility.py](../BootloaderCorePkg/Tools/IfwiUtility.py) |
| `HashStoreTable` / `HASH_USAGE` definitions | [SblOpen/BootloaderCorePkg/Tools/BuildUtility.py](../BootloaderCorePkg/Tools/BuildUtility.py) |
| Sign / re-sign a firmware-update capsule | [SblOpen/BootloaderCorePkg/Tools/GenCapsuleFirmware.py](../BootloaderCorePkg/Tools/GenCapsuleFirmware.py) |

## 12. Troubleshooting

| Symptom | Likely cause | Where to look |
|---|---|---|
| Stage1B halts with `HashStoreTable` verify failure | KEYH re-signed with wrong Master key, or Master key patched into Stage1A but algorithm mismatch | §7 (KEYH sign), §9 (digest length + algorithm) |
| Stage1A boots but Stage2 fails to load | Content hash mismatch — you edited a leaf you should not have (e.g., recompressed SG02 differently) | §5.3 (preserve compression), §8.2 |
| CFGDATA verify fails | `KEY_ID_CFGDATA_*` in KEYH does not match key used by `CfgDataTool.py sign`, or SVN downgraded | §7, §8.1 |
| Container header verify fails | `KEY_ID_CONTAINER_*` in KEYH does not match container header key | §7 (`PUBKEY_CONT_DEF`), §8.2 |
| Part refuses to boot at all (no serial output) | Most likely Boot Guard rejected the image (KM/BPM invalid, or fused OEM hash mismatch). Out of scope for this procedure — validate the output of `StitchIfwi.py` / `BtgSign.py` and silicon FPF programming | (see your platform stitch flow) |
| Boot works on TS0 but not after failover to TS1 | You forgot to mirror a change into TS1 | Re-do §8 and §9 for `IFWI/BIOS/TS1/*` leaves |
| Capsule update rejected in the field | New FWU key hash in KEYH does not match capsule signing key | §8.4 (transitional capsule plan) |
| `openssl` not found | `OPENSSL_PATH` unset and not on `PATH` | §4 |

## 13. What this procedure explicitly does not do

- Does **not** change the signing algorithm, key size, hash type, or
  signing scheme. Those are compiled into Stage1A/Stage1B.
- Does **not** change a container component's **compression algorithm**
  (LZ4 / LZMA / Dummy). Containers are re-signed in place — the
  compressed payload bytes are preserved byte-for-byte, and only the
  hash + signature bytes change. Recompression is a build-time concern.
- Does **not** add or remove `HASH_USAGE` slots in the on-flash
  HashStore. Only slots that existed in the original build can be
  rewritten.
- Does **not** program silicon fuses. FPF programming for a new Boot
  Guard OEM key hash is a separate, manufacturing-line activity.
- Does **not** sign OS payloads that live outside `SlimBootloader.bin`.
  Rotate those with your OS-image signing tooling using the new
  `KEY_ID_OS*_PRIVATE_*` keys.
- Does **not** re-sign Boot Guard (KM/BPM). Boot Guard keys are part
  of the platform/silicon trust chain, not the SBL key set — they are
  handled by the normal `StitchIfwi.py` / `BtgSign.py` flow that wraps
  the SBL image into a full IFWI. Re-run that flow on the resigned
  `SlimBootloader.bin` to produce the final deliverable.
