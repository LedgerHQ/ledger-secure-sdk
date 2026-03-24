# Address Book Flow

> **Note**: This is a standalone Markdown version of the Address Book flow documentation.
> For the Doxygen-generated documentation, see `mainpage.dox`.

## Command Flow: Add External Address / Ledger Account

```mermaid
sequenceDiagram
    participant Host as Host (Wallet)
    participant SDK as SDK (Address Book)
    participant App as Coin App
    participant User as User (Device)

    Note over Host,User: 1. Request Phase (Host → Device)

    Host->>SDK: APDU Command<br/>TLV Payload (signed by Ledger PKI)
    Note right of Host: TLV Structure:<br/>• structure_type (0x11/0x12)<br/>• version<br/>• contact_name<br/>• address_name (ext) / account_name (ledger)<br/>• derivation_path<br/>• address_raw (ext only)<br/>• derivation_scheme (ledger only)<br/>• chain_id<br/>• blockchain_family<br/>• DER signature

    SDK->>SDK: Parse TLV
    SDK->>SDK: Verify PKI signature<br/>(SECP256R1)

    alt Signature Invalid
        SDK-->>Host: Error: Security Violation
    end

    Note over SDK,App: 2. Application Validation

    SDK->>App: handle_check_external_address()<br/>Delegate address validation
    Note right of App: App-specific logic:<br/>• Verify address format<br/>• Check blockchain family<br/>• Validate chain_id

    App-->>SDK: Valid / Invalid

    alt Address Invalid
        SDK-->>Host: Error: Invalid Address
    end

    Note over User: 3. User Confirmation

    SDK->>User: Display Review Screen<br/>• Contact name<br/>• Address name<br/>• Address<br/>• Blockchain/Chain ID

    User-->>SDK: Confirm / Reject

    alt User Rejects
        SDK-->>Host: Error: User Rejected
    end

    Note over SDK: 4. Response Construction

    SDK->>SDK: Derive BIP32 key (SECP256R1)<br/>from derivation_path
    SDK->>SDK: Build name_payload<br/>len(1) | contact_name
    SDK->>SDK: Build remaining_payload<br/>WITHOUT type field
    SDK->>SDK: Derive AES-256 key<br/>from private key
    SDK->>SDK: Encrypt name<br/>(AES-256-GCM)
    SDK->>SDK: Encrypt remaining<br/>(AES-256-GCM)

    Note over Host,SDK: 5. Response Phase (Device → Host)

    SDK-->>Host: APDU Response<br/>type(1) | name_block | remaining_block

    Note left of SDK: Response Structure:<br/>• type (0x11/0x12) CLEARTEXT<br/>• name_encrypted_block:<br/>  - len(1)<br/>  - ciphertext(≤33)<br/>  - len_iv(1) + iv(12)<br/>  - len_tag(1) + tag(16)<br/>• remaining_encrypted_block:<br/>  - len(1)<br/>  - ciphertext(≤108)<br/>  - len_iv(1) + iv(12)<br/>  - len_tag(1) + tag(16)
```

## Data Flow Summary

### Input (Host → Device)

```text
TLV Payload (Signed by Ledger PKI)
├── structure_type: 0x11 (External) or 0x12 (Ledger)
├── version: 0x01
├── contact_name: string (max 32 bytes)
├── address_name/account_name: string (max 32 bytes)
├── derivation_path: BIP32 path
├── [address_raw: binary (max 64 bytes)] - External only
├── [derivation_scheme: uint8] - Ledger accounts only
├── [chain_id: uint64] - Required for Ethereum family
├── [blockchain_family: uint8] - Required for all types
└── signature: DER ECDSA (70-72 bytes)
```

### SDK Processing

1. **Parse & Verify**: TLV parsing + PKI signature verification
2. **Delegate**: Call app-specific validation (`handle_check_external_address`)
3. **UI**: Display review screen for user confirmation
4. **Crypto**:
   - Derive SECP256R1 key from `derivation_path`
   - Derive AES-256 key from private key
   - Encrypt both payloads with AES-256-GCM

### Output (Device → Host)

```text
APDU Response
├── type: 0x11 or 0x12 (CLEARTEXT)
├── name_encrypted_block:
│   ├── len(1)
│   ├── ciphertext: AES-GCM(len(1) | name)
│   ├── len_iv(1) + iv(12)
│   └── len_tag(1) + tag(16)
└── remaining_encrypted_block:
    ├── len(1)
    ├── ciphertext: AES-GCM(remaining_data)
    │   External: len(1) | address_name | len(1) | address_raw | chain_id(8) | family(1)
    │   Ledger: len(1) | derivation_path(40) | derivation_scheme(1) | chain_id(8) | family(1)
    ├── len_iv(1) + iv(12)
    └── len_tag(1) + tag(16)
```

## Key Design Decisions

1. **Type in Cleartext**: Host can identify message type before decryption
2. **Dual Encryption**: Name and remaining data encrypted separately for modularity
3. **BIP32 Derivation**: Path used for encryption key derivation
4. **PKI Verification**: Input verified with Ledger PKI (SECP256R1)
5. **App Validation**: Coin-specific logic delegated to app via callback
6. **User Confirmation**: Mandatory review screen before sending response
