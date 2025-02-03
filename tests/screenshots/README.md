# Screenshots generation

## Prerequisite

TBC

## Overview

The goal of this mechanism is to generate screenshots of usual Applications scenarios.

It can be used for any product (Stax, Flex, NanoX, NanoS+)

## Launch screenshots generation

The screenshots generator can be built and launched in the same command, for a given product

```
make <product>_screenshots
```

where `<product>` can be:

- `flex`
- `stax`
- `nanox`
- `nanosp`

so for example to generate screenshots for Flex:

```
make flex_screenshots
```

The result can be found in `build/<product>/screenshots`

## Clean

The environment can be cleaned-up with:

```
make <product>_clean
```
