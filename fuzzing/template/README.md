# App Fuzzing Template

Minimal scaffold to plug a Ledger app into the SDK fuzz framework. Copy
this directory to `<APP_SRC>/fuzzing/`, then fill the TODOs below. The
contract this template implements is documented in
[`../docs/APP_CONTRACT.md`](../docs/APP_CONTRACT.md); operational details
(campaign flags, environment, manual workflow) live in
[`../docs/CAMPAIGN_WORKFLOW.md`](../docs/CAMPAIGN_WORKFLOW.md).

## Onboarding checklist

1. Copy `fuzzing/template/` to `<APP_SRC>/fuzzing/`.
2. `fuzz-manifest.toml`: set CLA, INS list, `key_files`, and any
   dictionary tokens.
3. `CMakeLists.txt`: project name, source globs, include directories,
   compile definitions.
4. `harness/fuzz_dispatcher.c`: command table, `fuzz_app_reset()`,
   `fuzz_app_dispatch()`.
5. `mock/mocks.h` / `mock/mocks.c`: only if the app needs extra globals
   or app-specific mocks beyond the defaults shipped here.
6. Run the first campaign:

   ```bash
   BOLOS_SDK=/path/to/ledger-secure-sdk \
     "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
     --app-dir /path/to/your-app first-run
   ```

7. After the run, populate `invariants/zero-symbols.txt` with globals you
   want stripped from the prefix and (optionally) tighten enum domains in
   `invariants/domain-overrides.txt`.

The campaign builds the app, syncs the invariant, refreshes
`scenario_layout.h`, generates seeds, fuzzes, and writes a coverage
report under `<app>/.fuzz-artifacts/<run-name>/`.

## Reference

- Filled standard example: `app-boilerplate/fuzzing/`.
- Filled advanced example: `app-bitcoin-new/fuzzing/`.
- App contract: [`../docs/APP_CONTRACT.md`](../docs/APP_CONTRACT.md).
- Campaign / manual workflow:
  [`../docs/CAMPAIGN_WORKFLOW.md`](../docs/CAMPAIGN_WORKFLOW.md).
- SDK mock layer: [`../mock/README.md`](../mock/README.md).
