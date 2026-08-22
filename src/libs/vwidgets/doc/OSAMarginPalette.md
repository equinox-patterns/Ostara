# OSAMarginPalette

## A. Overview

`OSAMarginPalette` is the single source of truth for the seam-allowance margin
highlight colors. It exposes the 9 pen-style colors used for seam-allowance
margin highlights (Brown, Dark Blue, Dark Green, Light Blue, Light Green,
Orange, Pink, Purple, Yellow — the pen palette minus Black/White), each with a
light and a dark rendering.

The palette is **positional**: `colorIndex` values persisted in the pattern XML
(`saMarginProperties` group) and in the saved seam-allowance settings index into
`ColorNames()`. The palette must stay ordered and fixed as long as those indices
are persisted; only append at the end and bump `kColorCount`.

## B. Namespaces

| Namespace | Description |
|-----------|-------------|
| `OSAMarginPalette` | Free functions and constants describing the seam-allowance highlight palette. |

## C. Types and Type Aliases

None. The palette is expressed through constants and functions only.

## D. Constants

| Name | Type / Value | Description |
|------|--------------|-------------|
| `kColorCount` | `constexpr int` = `9` | Number of palette entries. Used for auto-assignment (`m_rows.size() % kColorCount`) and bounds checks on persisted `colorIndex` values. Sized at compile time by the `kColorKeys` array in `osamarginpalette.cpp` — adding/removing an entry without updating `kColorCount` is a compile error, so the two can never drift. |

## E. Functions

#### `ColorNames() -> const QStringList &`

Returns the pen color names in display order (the string values of the `ColorXxx`
constants from `ifcdef.h` — `brown`, `darkBlue`, `darkGreen`, `lightBlue`,
`lightGreen`, `orange`, `pink`, `purple`, `yellow`). This is the positional index
space used by the XML and settings `colorIndex`. The returned reference is a
function-local static, built on first call (after all globals are initialized),
and is valid for the lifetime of the application.

#### `Colors() -> QVector<QColor>`

Returns the 9 colors resolved for the **current theme** (`VTheme::ColorSheme()`).
Re-resolves on every call and must never be cached — a Light/Dark toggle must be
reflected immediately. Used to populate color swatch combos.

#### `Color(int index) -> QColor`

Returns the color at `index`, resolved for the current theme. Out-of-range indices
(negative or `>= kColorCount`) clamp to `0` (Brown). Used by
`OSAMarginHighlights` to pen the canvas highlight items.

## F. Dependencies

| Include | Provides |
|---------|----------|
| `../ifc/ifcdef.h` | The `ColorXxx` name constants and `ColorNameToQColorForTheme(name, dark)`, which resolves the light/dark hex value. |
| `../vtheme/vtheme.h` | `VTheme::ColorSheme()` (current theme) and `VColorSheme::Dark`. |

Build: `VWidgetsLib` (`src/libs/vwidgets/CMakeLists.txt`), which already links
`VThemeLib` and reaches `IFCLib` transitively.

## G. Usage Example

```cpp
// Populate a swatch combo.
for (const QColor &c : OSAMarginPalette::Colors())
{
    QPixmap px(16, 16);
    px.fill(c);
    combo->addItem(QIcon(px), QString());
}

// Pen a highlight item with the theme-aware color for a stored index.
item->setPen(QPen(OSAMarginPalette::Color(savedColorIndex), 2));
```
