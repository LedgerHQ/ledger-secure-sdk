# Address Book Flow

> **Note**: This is a standalone Markdown version of the Address Book flow documentation.
> For the Doxygen-generated documentation, see `mainpage.dox`.

## Sub-Commands Overview

The Address Book APDU (CLA=0xB0, INS=0x10) dispatches on P1:

| P1   | Sub-command             | Struct type |
|------|-------------------------|-------------|
| 0x01 | Register Identity       | 0x11        |
| 0x02 | Rename Identity         | 0x12        |
| 0x03 | Register Ledger Account | 0x13        |
| 0x04 | Rename Ledger Account   | 0x14        |

---

## Command Flow: Register Identity (P1=0x01)

```mermaid
sequenceDiagram
    participant Host as Host (Wallet)
    participant SDK as SDK (Address Book)
    participant App as Coin App
    participant User as User (Device)

    Note over Host,User: 1. Request Phase (Host → Device)

    Host->>SDK: APDU (P1=0x01)<br/>TLV Payload
    Note right of Host: TLV Structure:<br/>• struct_type (0x11)<br/>• struct_version (0x01)<br/>• contact_name (≤32 B)<br/>• contact_scope (≤32 B)<br/>• identifier (≤80 B)<br/>• derivation_path<br/>• chain_id (Ethereum only)<br/>• blockchain_family

    SDK->>SDK: Parse TLV
    SDK->>SDK: Verify mandatory fields

    Note over SDK,App: 2. Application Validation

    SDK->>App: handle_check_identity()
    Note right of App: App-specific logic:<br/>• Verify identifier format<br/>• Check blockchain family<br/>• Validate chain_id

    App-->>SDK: Valid / Invalid

    alt Identity Invalid
        SDK-->>Host: Error: Wrong Parameter Value
    end

    Note over User: 3. User Confirmation

    SDK->>User: Display Review Screen<br/>• Contact name<br/>• Contact scope<br/>• Identifier<br/>• Network / Chain ID

    User-->>SDK: Confirm / Reject

    alt User Rejects
        SDK-->>Host: Error: User Rejected
    end

    Note over SDK: 4. HMAC Proof Construction

    SDK->>SDK: Derive BIP32 key (SECP256R1)<br/>from derivation_path
    SDK->>SDK: KDF: SHA256("AddressBook-Identity" || privkey.d)
    SDK->>SDK: HMAC-SHA256(key, name_len|name|scope_len|scope|id_len|identifier)

    Note over Host,SDK: 5. Response Phase (Device → Host)

    SDK-->>Host: APDU Response<br/>type(1) | hmac_proof(32)

    Note left of SDK: Response Structure:<br/>• type (0x11) CLEARTEXT<br/>• hmac_proof: 32 bytes
```

---

## Command Flow: Register Ledger Account (P1=0x03)

```mermaid
sequenceDiagram
    participant Host as Host (Wallet)
    participant SDK as SDK (Address Book)
    participant App as Coin App
    participant User as User (Device)

    Note over Host,User: 1. Request Phase (Host → Device)

    Host->>SDK: APDU (P1=0x03)<br/>TLV Payload
    Note right of Host: TLV Structure:<br/>• struct_type (0x13)<br/>• struct_version (0x01)<br/>• contact_name (≤32 B)<br/>• derivation_path<br/>• chain_id (Ethereum only)<br/>• blockchain_family

    SDK->>SDK: Parse TLV
    SDK->>SDK: Verify mandatory fields

    Note over SDK,App: 2. Application Validation

    SDK->>App: handle_check_ledger_account()
    Note right of App: App-specific logic:<br/>• Verify derivation path<br/>• Validate chain_id

    App-->>SDK: Valid / Invalid

    alt Account Invalid
        SDK-->>Host: Error: Wrong Parameter Value
    end

    Note over User: 3. User Confirmation

    SDK->>User: Display Review Screen<br/>• Account name<br/>• Derived address<br/>• Network / Chain ID

    User-->>SDK: Confirm / Reject

    alt User Rejects
        SDK-->>Host: Error: User Rejected
    end

    Note over SDK: 4. HMAC Proof Construction

    SDK->>SDK: Derive BIP32 key (SECP256R1)<br/>from derivation_path
    SDK->>SDK: KDF: SHA256("AddressBook-LedgerAccount" || privkey.d)
    SDK->>SDK: HMAC-SHA256(key, name_len|name|family(1)|chain_id_be(8))

    Note over Host,SDK: 5. Response Phase (Device → Host)

    SDK-->>Host: APDU Response<br/>type(1) | hmac_proof(32)

    Note left of SDK: Response Structure:<br/>• type (0x13) CLEARTEXT<br/>• hmac_proof: 32 bytes
```

---

## Data Flow Summary

### TLV Tags

| Tag  | Name                  | Description                               | Max size |
|------|-----------------------|-------------------------------------------|----------|
| 0x01 | STRUCT_TYPE           | Sub-command type (0x11–0x14)              | 1 B      |
| 0x02 | STRUCT_VERSION        | Always 0x01                               | 1 B      |
| 0x0A | CONTACT_NAME          | Human-readable contact/account name       | 32 B     |
| 0x0B | CONTACT_SCOPE         | Scope/namespace for the identifier        | 32 B     |
| 0x0C | PREVIOUS_CONTACT_NAME | Previous name (Rename commands only)      | 32 B     |
| 0x0F | IDENTIFIER            | Opaque identifier (e.g. Ethereum address) | 80 B     |
| 0x21 | DERIVATION_PATH       | BIP32 path for HMAC key derivation        | 43 B     |
| 0x23 | CHAIN_ID              | Chain ID (mandatory for Ethereum family)  | 8 B      |
| 0x26 | HMAC_PROOF            | HMAC proof (Rename commands only)         | 32 B     |
| 0x51 | BLOCKCHAIN_FAMILY     | Blockchain family enum                    | 1 B      |

### Register Identity Input (P1=0x01)

```text
TLV Payload
├── struct_type:       0x11
├── struct_version:    0x01
├── contact_name:      string (max 32 bytes, printable ASCII)
├── contact_scope:     string (max 32 bytes, printable ASCII)  ← mandatory
├── identifier:        binary (max 80 bytes)
├── derivation_path:   BIP32 path
├── [chain_id:         uint64]   ← mandatory for FAMILY_ETHEREUM
└── blockchain_family: uint8
```

### Register Ledger Account Input (P1=0x03)

```text
TLV Payload
├── struct_type:       0x13
├── struct_version:    0x01
├── contact_name:      string (max 32 bytes, printable ASCII)
├── derivation_path:   BIP32 path
├── [chain_id:         uint64]   ← mandatory for FAMILY_ETHEREUM
└── blockchain_family: uint8
```

### SDK Processing

1. **Parse**: TLV parsing and mandatory field verification
2. **Delegate**: Call app-specific validation callback
3. **UI**: Display review screen for user confirmation
4. **Crypto**:
   - Derive SECP256R1 private key from `derivation_path`
   - KDF: `SHA256(salt || privkey.d)` where salt is "AddressBook-Identity" or "AddressBook-LedgerAccount"
   - `HMAC-SHA256(key, message)` — message depends on sub-command (see below)

### HMAC Message Construction

**Identity (0x11)**:

```text
name_len(1) | contact_name | scope_len(1) | contact_scope | id_len(1) | identifier
```

**Ledger Account (0x13)**:

```text
name_len(1) | account_name | blockchain_family(1) [| chain_id_be(8)]
```

`chain_id_be(8)` is included only for `FAMILY_ETHEREUM`.

### Output (Device → Host)

```text
APDU Response
├── type:       0x11 / 0x12 / 0x13 / 0x14 (CLEARTEXT)
└── hmac_proof: 32 bytes (HMAC-SHA256)
```

---

## Key Design Decisions

1. **Type in Cleartext**: Host can identify the sub-command before processing the response
2. **HMAC Proof**: Cryptographically binds the user's on-device confirmation to the
   (name, scope/path, identifier/family, chain_id) tuple
3. **Dual KDF**: Separate salts for Identity and Ledger Account prevent cross-feature
   key reuse even at the same BIP32 derivation path
4. **No secrets leave the device**: BIP32 private key and HMAC key are computed
   on-device and never transmitted to the host
5. **App Validation**: Coin-specific logic delegated to the application via callback
6. **User Confirmation**: Mandatory review screen before the HMAC proof is sent
