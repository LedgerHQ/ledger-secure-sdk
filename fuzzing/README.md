# Ledger SDK Fuzzing Framework

Coverage-guided fuzzing for the Ledger Secure SDK and Ledger apps, built on
LibFuzzer and Clang sanitizers.

The full documentation — concepts, running a campaign, integrating an app, and
maintaining it — is the **Fuzzing Framework** page of the SDK documentation
(source: [`doc/mainpage.dox`](doc/mainpage.dox)), published at
<https://ledgerhq.github.io/ledger-secure-sdk/>.

## Quickstart

Run a campaign against an app that ships a `fuzzing/` folder:

```bash
BOLOS_SDK=/path/to/ledger-secure-sdk \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir /path/to/app my-campaign
```

New integrations start from the `fuzzing/` folder in
[app-boilerplate](https://github.com/LedgerHQ/app-boilerplate).
