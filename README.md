# Ledger Secure SDK

## Are you developing an application?

If you are developing an application, for a smooth and quick integration:
- See the developers’ documentation on the [Developer Portal](https://developers.ledger.com/)
- [Go on Discord](https://developers.ledger.com/discord-pro/) to chat with developer support and the developer community.

## Introduction

This directory contains the SDK for Nano X, Nano S+, Stax, Apex+ and Flex applications development.

This SDK is tightly linked to the Ledger Hardware Wallet OS: BOLOS.

Indeed, it allows interacting with `syscalls` and `cxlib` functions which are embedded in the OS.

Hence you should make sure to use the right SDK version matching your development device OS.

You can find below two possibilities to build against the right SDK version.

## Using the docker image

The easiest way to build against last device OS is to use the docker image provided by ledger and accessible on ghcr.io.

The corresponding GIT repository can be found [here](https://github.com/LedgerHQ/ledger-app-builder/). Please have a look at its `README.md` for information about its usage.

## Using the SDK directly (advanced users)

Advanced users that have setup the Ledger development environment can use this repository to build apps.

But for that, they need to understand how OS and SDK compatibility are tracked. This is done with the `API_LEVEL` which is defined in `Makefile.defines`.

The `API_LEVEL` on `master` branch is kept as the reserved value `0`.

For each released OS there is a corresponding tag in the format `<device>_<os_version>`, e.g. `nanox_2.8.0` for the release of the OS version `2.8.0` for Nano X device. While on this tag, if you look at the value of the `API_LEVEL` which is defined in `Makefile.defines` you will retrieve the OS `API_LEVEL`.

There are also `API_LEVEL_<N>` branches with `API_LEVEL` value set to `N`. Their purpose is to allow cherry-picks of bug fixes and improvements that are merged on `master` so that they are available when building the apps for the corresponding OS.

On these `API_LEVEL_<N>` branches, there are tags following the format `v<N>.<minor>.<patch>`, e.g. `v1.1.0` where `N` is the `API_LEVEL`. These tags are used to generate the `SDK_VERSION` which is available at compile time and allows tracking the SDK version used to build an app.

The branch `API_LEVEL_LNS` tracks the SDK for `nanos_2.1.0`. Its naming does not follow the `API_LEVEL_<N>` format because it is not a child of the branch `master` (`API_LEVEL = 0`).

In short, to build an app for an OS, you should:
- Retrieve the OS `API_LEVEL`:
  - `git checkout <device>_<os_version>`
  - `grep API_LEVEL Makefile.defines | head -n1`
- Check out the `API_LEVEL_<N>` branch related to the OS `API_LEVEL` and make sure it is up to date:
  - `git checkout API_LEVEL_<N>`
  - `git pull`
  - The last commit should be tagged with the complete version of the SDK (`v<N>.<x>.<y>`)
- Build the app from your app folder:
  - `make BOLOS_SDK=<path_to_sdk> TARGET=<target>` where `target` is one of `nanox`, `nanos2`, `stax`, `flex`, `apex_p` (`nanos2` is used for Nano S+ device).

## About API_LEVEL branches

This lists the main API_LEVEL branches with their purpose (corresponding OS) and state if they should still be patched or not (OS not “active” anymore).

The full mapping of API_LEVEL branches, including OS release candidates, is available [here](api_levels.json).

| Name | Related OS                                                                                                                                     | Active             |
| ---- | ---------------------------------------------------------------------------------------------------------------------------------------------- | -------------------|
| LNS  | <br/> nanos_2.1.0                                                                                                                              | :heavy_check_mark: |
| 1    | nanox_2.1.0 <br/> nanos+_1.1.0                                                                                                                 | :x:                |
| 5    | nanox_2.2.{0, 1, 2, 3} <br/> nanos+ 1.1.1                                                                                                      | :x:                |
| 8    | stax_1.0.0                                                                                                                                     | :x:                |
| 10   | stax_1.1.0                                                                                                                                     | :x:                |
| 11   | stax_1.2.0 <br/> stax_1.2.1                                                                                                                    | :x:                |
| 13   | stax_1.3.0                                                                                                                                     | :x:                |
| 15   | stax_1.4.0                                                                                                                                     | :x:                |
| 18   | nanos+_1.2.0                                                                                                                                   | :x:                |
| 19   | flex_1.0.0 <br/> flex_1.0.1 <br/>                                                                                                              | :x:                |
| 21   | stax_1.5.0 <br/> flex_1.1.0 <br/> flex_1.1.1 <br/>                                                                                             | :x:                |
| 22   | nanox_2.4.1 <br/> nanos+_1.3.1 <br/> stax_1.6.1 <br/> flex_1.2.1 <br/> nanox_2.4.2 <br/> nanos+_1.3.2 <br/> stax_1.6.2 <br/> flex_1.2.2 <br/>  | :x:                |
| 24   | nanox_2.5.1 <br/> nanos+_1.4.1 <br/> stax_1.8.1 <br/> flex_1.4.1 <br/>                                                                         | :x:                |
| 25   | apex_p_1.0.4 <br/> nanox_2.6.0 <br/> nanos+_1.5.0 <br/> stax_1.9.0 <br/> flex_1.5.0 <br/>                                                      | :x:                |
| 26   | apex_p_1.1.1 <br/> nanox_2.7.1 <br/> nanos+_1.6.1 <br/> stax_1.10.1 <br/> flex_1.6.1 <br/>                                                     | :x:                |
| 27   | apex_p_1.2.0 <br/> nanox_2.8.0 <br/> nanos+_1.7.0 <br/> stax_1.11.0 <br/> flex_1.7.0 <br/>                                                     | :heavy_check_mark: |

### Cherry-picking process

#### Automatic cherry-pick

When a PR is merged on `master`, the [auto-cherry-pick](.github/workflows/auto-cherry-pick.yml) workflow
reads its description and looks for checked lines like this one (from the PR template):

```text
[x] TARGET_API_LEVEL: API_LEVEL_27
```

- Check the box(es) **before merging**: the description is read when the PR is merged.
- The line must start with `[x]`, so do not turn it into a list item (`- [x] ...` is ignored).
  Add one line per target branch; each `API_LEVEL_<N>` branch must already exist.
- For each target, the PR commits are cherry-picked (with `-x`) onto an `auto_update_API_LEVEL_<N>` branch,
  and a PR `[AUTO_UPDATE] Branch API_LEVEL_<N>` is opened, with the original author as reviewer.
  If such a PR is already open, the new commits are added to it.
- The merge of this PR is always manual. Its branch is deleted once it is closed.
- The PR's own commits are cherry-picked one by one, so a PR containing a merge commit (e.g. `master` merged
  into the PR branch) cannot be cherry-picked: rebase the PR branch instead.
- If a cherry-pick does not apply (conflict, missing previous commit), nothing is pushed and the workflow run fails:
  fall back to the manual process below.

The workflow can also be run manually (`workflow_dispatch`) with a PR number and a target branch,
for instance when the box was forgotten before the merge.

#### Manual cherry-pick

- Fetch last changes from remote: `git fetch --all`

- Create a new branch to hold your cherry-picks: `git checkout origin/API_LEVEL_X -b mybranch`

- Cherry-pick your commits: `git cherry-pick -x commit_sha1` (the -x is useful to track the original commit of a cherry-pick).

- Push your branch: `git push origin mybranch`

- Create a PR and indicate in it the PR where your cherry-picks were reviewed first.

## Contributing

### Pre-commit

This repository uses [pre-commit](https://pre-commit.com/) to identify simple programming issues at the time of code check-in.

To enable pre-commit in your development environment:

1. Install pre-commit:

    ```shell
    pip install pre-commit
    ```

2. Add pre-commit hooks

    ```shell
    pre-commit install --hook-type pre-commit
    pre-commit install --hook-type commit-msg
    ```

## Documentation

HTML documentation can be generated from the root directory with:

```shell
make doc-wallet   # touchscreen devices (Stax, Flex, Apex P), same as `make doc TARGET=stax`
make doc-nano     # Nano X / Nano S+, same as `make doc TARGET=nanox`
make doc-all      # both, plus a landing page
```

`doc-wallet` and `doc-nano` write to `build/doc/html/index.html`; `doc-all` writes to `build/doc/site/index.html`.

Dependencies (Ubuntu):

```shell
sudo apt-get install doxygen graphviz default-jre
```

`default-jre` is only needed to render the PlantUML diagrams.
