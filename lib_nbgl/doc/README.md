# NBGL Documentation — Diagram Sources

## Structure

```text
doc/
├── sources/      ← Excalidraw source files (.excalidraw) — versioned in git
├── resources/    ← Exported PNG files — generated artefacts
└── *.dox         ← Doxygen source files referencing PNGs from resources/
```

The `.excalidraw` files are the **source of truth**. The PNGs in `resources/` are
derived artefacts and must be regenerated when sources change.

---

## Editing diagrams

### Prerequisites

Install the [Excalidraw extension for VS Code](https://marketplace.visualstudio.com/items?itemName=pomdtr.excalidraw-editor)
(extension id: `pomdtr.excalidraw-editor`). This gives a native editor directly in the IDE.

Alternatively, open any `.excalidraw` file at [excalidraw.com](https://excalidraw.com):
**Menu → Open** → select the file.

### Conventions used across diagrams

| Element | Style |
| --- | --- |
| screen frame | `strokeColor: #cccccc`,<br>`backgroundColor: #ffffff`,<br>`roundness: {type:3, value:6}` |
| Wide popup frame (Confirm, Keypad…) | `strokeColor: #000000`,<br>`backgroundColor: #ffffff`,<br>`roundness: {type:3, value:6}` |
| Blue cursive title | `strokeColor: #1971c2`,<br>`fontFamily: 5` (cursive),<br>`fontSize: 36` |
| Annotation box | `strokeColor: #e03131`,<br>`strokeStyle: dashed`,<br>`backgroundColor: transparent` |
| Annotation arrow | `strokeColor: #e03131`,<br>`endArrowhead: arrow` |
| App/callback ellipse | `strokeColor: #e07020` (orange) |
| Callback arrow (purple) | `strokeColor: #7950f2` |
| Navigation back arrow (green) | `strokeColor: #1c7a1c` |
| Toggle switch | rounded rect `backgroundColor: #dddddd` + ellipse knob `backgroundColor: #ffffff` |

### Adding a new diagram

1. Create `sources/<Name>.excalidraw` following the conventions above.
2. Export a PNG (see below) to `resources/<Name>.png`.
3. Reference it in the relevant `.dox` file with `\image html <Name>.png`.

---

## Exporting PNGs

### Manual export

**From VS Code** (Excalidraw extension):
- Open the `.excalidraw` file.
- Open the Command Palette (`Ctrl+Shift+P`) → **Excalidraw: Export to PNG**.
- Save the file to `resources/`.

**From excalidraw.com**:
- Open the file via **Menu → Open**.
- **Menu → Export image** → PNG, background: on, scale: 2×.
- Save to `resources/`.
