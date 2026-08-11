#!/usr/bin/env python3
"""
Seed corpus generator for fuzz_address_book.

Writes one valid-TLV binary seed file per address-book command variant into
the directory passed as argv[1].  The TLV values are fake but syntactically
correct, so the fuzzer starts directly inside the business-logic paths rather
than spending its entire budget on TLV parsing failures.

Harness seed layout (from fuzz_address_book.c / fuzz_harness.h):
  byte[0..ps-1]  Absolution globals prefix (size resolved from build output)
  byte[ps+0]     ctrl   — 0x00 keeps the raw lane (ctrl ≤ 102)
  byte[ps+1]     cmd    — command-table index (always 0, only one spec)
  byte[ps+2]     P1     — sub-command selector (clamped to 0x21 by harness)
  byte[ps+3]     P2     — 0x00 = single-chunk, 0x80 = two-chunk split
  byte[ps+4+]    tail   — raw TLV payload; harness prepends the 2-byte length
                          header automatically before calling addr_book_handle_apdu
"""

import os
import sys
import struct

# Make fuzz_seed_utils importable from the sibling scripts/ directory.
_SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.realpath(os.path.join(_SCRIPT_DIR, "..", "scripts")))

from fuzz_seed_utils import (
    resolve_prefix_size,
    resolve_seed_prefix,
    ctrl_bytes,
)

# ── TLV tag byte values ──────────────────────────────────────────────────────
TAG_STRUCT_TYPE     = 0x01
TAG_STRUCT_VERSION  = 0x02
TAG_CHAIN_ID        = 0x23
TAG_HMAC_PROOF      = 0x29
TAG_BLOCKCHAIN_FAM  = 0x51
TAG_DERIV_PATH      = 0x69
TAG_CONTACT_NAME    = 0xf0
TAG_SCOPE           = 0xf1
TAG_IDENTIFIER      = 0xf2
TAG_PREV_NAME       = 0xf3
TAG_PREV_IDENTIFIER = 0xf4
TAG_PREV_SCOPE      = 0xf5
TAG_GROUP_HANDLE    = 0xf6
TAG_HMAC_REST       = 0xf7

# ── Struct type constants ────────────────────────────────────────────────────
TYPE_REGISTER_IDENTITY           = 0x2d
TYPE_EDIT_CONTACT_NAME           = 0x2e
TYPE_REGISTER_LEDGER_ACCOUNT     = 0x2f
TYPE_EDIT_LEDGER_ACCOUNT         = 0x30
TYPE_EDIT_IDENTIFIER             = 0x31
TYPE_EDIT_SCOPE                  = 0x32
TYPE_PROVIDE_CONTACT             = 0x33
TYPE_PROVIDE_LEDGER_ACCT_CONTACT = 0x34

# ── P1 sub-command selectors ─────────────────────────────────────────────────
P1_REGISTER_IDENTITY         = 0x01
P1_EDIT_CONTACT_NAME         = 0x02
P1_EDIT_IDENTIFIER           = 0x03
P1_EDIT_SCOPE                = 0x04
P1_REGISTER_LEDGER_ACCOUNT   = 0x11
P1_EDIT_LEDGER_ACCOUNT       = 0x12
P1_PROVIDE_CONTACT           = 0x20
P1_PROVIDE_LEDGER_ACCT       = 0x21

# ── Shared fake test data ────────────────────────────────────────────────────
# BIP32 path  m/44'/60'/0'/0/0  (5 components, big-endian uint32)
BIP32_PATH_BTC = bytes([0x05,
    0x80, 0x00, 0x00, 0x2C,  # 44' (Bitcoin purpose)
    0x80, 0x00, 0x00, 0x00,  # 0' (coin type)
    0x80, 0x00, 0x00, 0x00,  # 0' (account)
    0x00, 0x00, 0x00, 0x00,  # 0 (change)
    0x00, 0x00, 0x00, 0x00,  # 0 (address index)
])
BIP32_PATH_ETH = bytes([0x05,
    0x80, 0x00, 0x00, 0x2C,  # 44'
    0x80, 0x00, 0x00, 0x3C,  # 60' (Ethereum coin type)
    0x80, 0x00, 0x00, 0x00,  # 0'
    0x00, 0x00, 0x00, 0x00,  # 0
    0x00, 0x00, 0x00, 0x00,  # 0
])

FAMILY_BITCOIN  = bytes([0x00])
FAMILY_ETHEREUM = bytes([0x01])

# Ethereum mainnet chain ID = 1 (big-endian uint64, 8-byte encoding)
CHAIN_ID_ETH_MAINNET = struct.pack(">Q", 1)
# Polygon mainnet chain ID = 137 — encoded as 1 byte to exercise the short-TLV path
# in get_uint64_t_from_tlv_data (the parser accepts 1–8 byte values).
CHAIN_ID_POLYGON = bytes([0x89])
# Maximum uint64 value — exercises the upper boundary of the 8-byte parsing path.
CHAIN_ID_MAX = struct.pack(">Q", 2**64 - 1)

# Fake 20-byte identifier (looks like an Ethereum address)
IDENTIFIER_ETH = bytes(range(20))
# Fake 20-byte Bitcoin identifier
IDENTIFIER_BTC = bytes([0xAA] * 20)

# group_handle = gid(32) | MAC(32)  — fake, verification is mocked to pass
GROUP_HANDLE  = bytes([0x11] * 32 + [0x22] * 32)
GID           = bytes([0x11] * 32)  # first 32 bytes of GROUP_HANDLE

HMAC_PROOF    = bytes([0xAB] * 32)  # matches sys_address_book_hmac mock output
HMAC_REST     = bytes([0xAB] * 32)

NAME_ALICE    = b"Alice"
NAME_BOB      = b"Bob"
NAME_WALLET   = b"MyWallet"
NAME_WALLET2  = b"NewWallet"
SCOPE_BTC     = b"Bitcoin"
SCOPE_DEFI    = b"DeFi"


# ── TLV encoding helpers ──────────────────────────────────────────────────────

def tlv(tag: int, value: bytes) -> bytes:
    """Encode one TLV triplet (tag=1B, length=1B, value=N bytes)."""
    assert len(value) <= 255, f"value too long for single-byte length: {len(value)}"
    return bytes([tag, len(value)]) + value


def tlv_u8(tag: int, value: int) -> bytes:
    return tlv(tag, bytes([value]))


def tlv_str(tag: int, s: bytes) -> bytes:
    return tlv(tag, s)


def tlv_raw(tag: int, data: bytes) -> bytes:
    return tlv(tag, data)


# ── Seed-file writer ─────────────────────────────────────────────────────────

def write_seed(output_dir: str, name: str, p1: int, payload: bytes, prefix: bytes) -> None:
    """Write a seed file.  payload = raw TLV bytes (no length header needed)."""
    # Raw lane, first (only) command; P2=0 is the single-chunk case.
    data = prefix + ctrl_bytes(structured=False, cmd_idx=0, p1=p1, p2=0) + payload
    path   = os.path.join(output_dir, name)
    with open(path, "wb") as f:
        f.write(data)
    print(f"  wrote {name!r} ({len(data)} bytes, P1=0x{p1:02x})")


# ── Per-command seed builders ────────────────────────────────────────────────

def seeds_register_identity(out: str, prefix: bytes) -> None:
    """P1=0x01 — new group (BTC) + ETH variants for mainnet/Polygon/MAX chain IDs."""

    # Variant A: new identity, Bitcoin family
    write_seed(out, "register_identity_btc_new_group", P1_REGISTER_IDENTITY,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_IDENTITY) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_BTC) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_BTC) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00),
        prefix,
    )

    # Variant B: existing group handle, Ethereum mainnet (chain_id=1, 8-byte encoding)
    write_seed(out, "register_identity_eth_existing_group", P1_REGISTER_IDENTITY,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_IDENTITY) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_ETH) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_CHAIN_ID,      CHAIN_ID_ETH_MAINNET) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )

    # Variant C: Polygon (chain_id=137, 1-byte TLV — exercises short-encoding path)
    write_seed(out, "register_identity_polygon", P1_REGISTER_IDENTITY,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_IDENTITY) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_ETH) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_CHAIN_ID,      CHAIN_ID_POLYGON) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )

    # Variant D: UINT64_MAX chain_id — upper-boundary of 8-byte parsing path
    write_seed(out, "register_identity_chain_id_max", P1_REGISTER_IDENTITY,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_IDENTITY) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_ETH) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_CHAIN_ID,      CHAIN_ID_MAX) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )


def seeds_edit_contact_name(out: str, prefix: bytes) -> None:
    """P1=0x02 — all fields mandatory, no conditional ones."""

    write_seed(out, "edit_contact_name_btc", P1_EDIT_CONTACT_NAME,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_EDIT_CONTACT_NAME) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_BOB) +
        tlv_str(TAG_PREV_NAME,     NAME_ALICE) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )


def seeds_edit_identifier(out: str, prefix: bytes) -> None:
    """P1=0x03 — Bitcoin + ETH variants for mainnet/Polygon/MAX chain IDs."""
    base = (
        tlv_u8(TAG_STRUCT_TYPE,       TYPE_EDIT_IDENTIFIER) +
        tlv_u8(TAG_STRUCT_VERSION,    0x01) +
        tlv_str(TAG_CONTACT_NAME,     NAME_ALICE) +
        tlv_str(TAG_SCOPE,            SCOPE_BTC) +
        tlv_raw(TAG_IDENTIFIER,       IDENTIFIER_BTC) +
        tlv_raw(TAG_PREV_IDENTIFIER,  IDENTIFIER_BTC) +
        tlv_raw(TAG_GROUP_HANDLE,     GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,       BIP32_PATH_BTC) +
        tlv_raw(TAG_HMAC_PROOF,       HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,        HMAC_REST)
    )
    write_seed(out, "edit_identifier_btc", P1_EDIT_IDENTIFIER,
        base + tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00), prefix)

    eth_base = (
        tlv_u8(TAG_STRUCT_TYPE,       TYPE_EDIT_IDENTIFIER) +
        tlv_u8(TAG_STRUCT_VERSION,    0x01) +
        tlv_str(TAG_CONTACT_NAME,     NAME_ALICE) +
        tlv_str(TAG_SCOPE,            SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,       IDENTIFIER_ETH) +
        tlv_raw(TAG_PREV_IDENTIFIER,  IDENTIFIER_ETH) +
        tlv_raw(TAG_GROUP_HANDLE,     GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,       BIP32_PATH_ETH) +
        tlv_raw(TAG_HMAC_PROOF,       HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,        HMAC_REST) +
        tlv_u8(TAG_BLOCKCHAIN_FAM,    0x01)
    )
    write_seed(out, "edit_identifier_eth", P1_EDIT_IDENTIFIER,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_ETH_MAINNET), prefix)
    write_seed(out, "edit_identifier_polygon", P1_EDIT_IDENTIFIER,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_POLYGON), prefix)
    write_seed(out, "edit_identifier_chain_id_max", P1_EDIT_IDENTIFIER,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_MAX), prefix)


def seeds_edit_scope(out: str, prefix: bytes) -> None:
    """P1=0x04 — Bitcoin + ETH variants for mainnet/Polygon/MAX chain IDs."""
    write_seed(out, "edit_scope_btc", P1_EDIT_SCOPE,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_EDIT_SCOPE) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_BTC) +
        tlv_str(TAG_PREV_SCOPE,    SCOPE_BTC) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,     HMAC_REST) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00),
        prefix,
    )

    eth_base = (
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_EDIT_SCOPE) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_ETH) +
        tlv_str(TAG_PREV_SCOPE,    SCOPE_BTC) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,     HMAC_REST) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01)
    )
    write_seed(out, "edit_scope_eth", P1_EDIT_SCOPE,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_ETH_MAINNET), prefix)
    write_seed(out, "edit_scope_polygon", P1_EDIT_SCOPE,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_POLYGON), prefix)
    write_seed(out, "edit_scope_chain_id_max", P1_EDIT_SCOPE,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_MAX), prefix)


def seeds_provide_contact(out: str, prefix: bytes) -> None:
    """P1=0x20 — Bitcoin and Ethereum."""
    write_seed(out, "provide_contact_btc", P1_PROVIDE_CONTACT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_PROVIDE_CONTACT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_BTC) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_BTC) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,     HMAC_REST) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00),
        prefix,
    )

    write_seed(out, "provide_contact_eth", P1_PROVIDE_CONTACT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_PROVIDE_CONTACT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_ALICE) +
        tlv_str(TAG_SCOPE,         SCOPE_DEFI) +
        tlv_raw(TAG_IDENTIFIER,    IDENTIFIER_ETH) +
        tlv_raw(TAG_GROUP_HANDLE,  GROUP_HANDLE) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_CHAIN_ID,      CHAIN_ID_ETH_MAINNET) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_raw(TAG_HMAC_REST,     HMAC_REST) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01),
        prefix,
    )


def seeds_register_ledger_account(out: str, prefix: bytes) -> None:
    """P1=0x11 — Bitcoin (no CHAIN_ID) + ETH variants for mainnet/Polygon/MAX."""
    write_seed(out, "register_ledger_account_btc", P1_REGISTER_LEDGER_ACCOUNT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_LEDGER_ACCOUNT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00),
        prefix,
    )

    eth_base = (
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_REGISTER_LEDGER_ACCOUNT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01)
    )
    write_seed(out, "register_ledger_account_eth", P1_REGISTER_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_ETH_MAINNET), prefix)
    write_seed(out, "register_ledger_account_polygon", P1_REGISTER_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_POLYGON), prefix)
    write_seed(out, "register_ledger_account_chain_id_max", P1_REGISTER_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_MAX), prefix)


def seeds_edit_ledger_account(out: str, prefix: bytes) -> None:
    """P1=0x12 — Bitcoin + ETH variants for mainnet/Polygon/MAX chain IDs."""
    write_seed(out, "edit_ledger_account_btc", P1_EDIT_LEDGER_ACCOUNT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_EDIT_LEDGER_ACCOUNT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET2) +
        tlv_str(TAG_PREV_NAME,     NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00),
        prefix,
    )

    eth_base = (
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_EDIT_LEDGER_ACCOUNT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET2) +
        tlv_str(TAG_PREV_NAME,     NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01)
    )
    write_seed(out, "edit_ledger_account_eth", P1_EDIT_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_ETH_MAINNET), prefix)
    write_seed(out, "edit_ledger_account_polygon", P1_EDIT_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_POLYGON), prefix)
    write_seed(out, "edit_ledger_account_chain_id_max", P1_EDIT_LEDGER_ACCOUNT,
        eth_base + tlv_raw(TAG_CHAIN_ID, CHAIN_ID_MAX), prefix)


def seeds_provide_ledger_account_contact(out: str, prefix: bytes) -> None:
    """P1=0x21 — Bitcoin and Ethereum."""
    write_seed(out, "provide_ledger_account_btc", P1_PROVIDE_LEDGER_ACCT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_PROVIDE_LEDGER_ACCT_CONTACT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_BTC) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x00) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )

    write_seed(out, "provide_ledger_account_eth", P1_PROVIDE_LEDGER_ACCT,
        tlv_u8(TAG_STRUCT_TYPE,    TYPE_PROVIDE_LEDGER_ACCT_CONTACT) +
        tlv_u8(TAG_STRUCT_VERSION, 0x01) +
        tlv_str(TAG_CONTACT_NAME,  NAME_WALLET) +
        tlv_raw(TAG_DERIV_PATH,    BIP32_PATH_ETH) +
        tlv_raw(TAG_CHAIN_ID,      CHAIN_ID_ETH_MAINNET) +
        tlv_u8(TAG_BLOCKCHAIN_FAM, 0x01) +
        tlv_raw(TAG_HMAC_PROOF,    HMAC_PROOF),
        prefix,
    )


# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> None:
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <output_dir>", file=sys.stderr)
        sys.exit(1)

    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    print(f"generating address-book seeds → {out}")

    prefix_size = resolve_prefix_size(fuzzer_name="fuzz_address_book")
    # The prefix is opaque sampled state; control bytes live at the start of the
    # harness input, which write_seed() puts there.
    prefix = resolve_seed_prefix(prefix_size, fuzzer_name="fuzz_address_book")
    print(f"  prefix size: {prefix_size} bytes")

    seeds_register_identity(out, prefix)
    seeds_edit_contact_name(out, prefix)
    seeds_edit_identifier(out, prefix)
    seeds_edit_scope(out, prefix)
    seeds_provide_contact(out, prefix)
    seeds_register_ledger_account(out, prefix)
    seeds_edit_ledger_account(out, prefix)
    seeds_provide_ledger_account_contact(out, prefix)

    total = len(os.listdir(out))
    print(f"done — {total} seed file(s) written")


if __name__ == "__main__":
    main()
