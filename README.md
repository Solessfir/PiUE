# PiUE - Pie Menu for Unreal Editor

Blender-style radial quick-action menu for the Unreal Engine level editor viewport.

![Editor Screenshot](Resources/Screenshot.png)

### Installation
Get `PiUE.zip` from the [releases](https://github.com/Solessfir/PiUE/releases) and extract it into your project's `Plugins` folder.

## Usage

PiUE supports up to **five independent rings**, each bound to its own hotkey. Press the bound key to open that ring. **Tap** (< `Tap Threshold`) leaves the menu open - click a wedge or press again to close. **Hold** (≥ `Tap Threshold`) executes the highlighted wedge on release. Move the cursor away from center to highlight a wedge; stay in the dead zone to close without acting.

By default each ring only opens when the cursor is over the **level viewport**. Enable **Available Anywhere** on a ring to allow it to open in any editor window.

**Ring 1** defaults to **V** and **Mouse 4**. Rings 2 - 5 are unbound by default.

The menu is unavailable while Play In Editor is active.

> All ring bindings can be rebound in **Editor Preferences → General → Keyboard Shortcuts → PiUE**.

## Configuration

**Editor Preferences → Plugins → PiUE**

### Menu

Each ring has two settings:

| Property | Default | Description |
|----------|---------|-------------|
| **Items** | — | Wedges shown when this ring is opened. |
| **Available Anywhere** | `false` | When off, the ring only opens over the level viewport. When on, it opens in any editor window. |

### Input

| Property | Default | Unit | Description |
|----------|---------|------|-------------|
| **Tap Threshold** | `150` | ms | Short press leaves the menu open for click navigation. Long press executes the hovered wedge on release. |
| **Category Hover Ms** | `1000` | ms | How long a category wedge must be hovered before auto-navigating into it. |

### Layout

| Property | Default | Description |
|----------|---------|-------------|
| **Menu Radius** | `120` | Ring radius in screen pixels. |
| **Dead Zone Radius** | `25` | Cursor distance from center below which nothing is selected. |

### Animation

| Property | Default | Unit | Description |
|----------|---------|------|-------------|
| **Wedge Exit Duration** | `130` | ms | Duration of the wedge exit animation when the menu closes or navigates. |
| **Wedge Anim Speed** | `25` | ×/s | Speed multiplier for wedge enter/exit translation animation. Higher = snappier. |
| **Highlight Anim Speed** | `14` | ×/s | Speed multiplier for wedge highlight color transition. Higher = snappier. |
| **Arc Track Speed** | `18` | ×/s | Speed multiplier for the arc indicator tracking the hovered wedge. Higher = snappier. |
| **Arc Fade Speed** | `10` | ×/s | Speed multiplier for the arc indicator fade in/out. Higher = snappier. |

### Style

| Property | Default | Description |
|----------|---------|-------------|
| **Icon Picker Size** | `16` | Size of icons in the editor icon picker grid (pixels). |
| **Default Wedge Tint** | — | Background color for unselected wedges. |
| **Highlight Wedge Tint** | — | Background color for the hovered wedge. |

## Item Types

All item types share these base properties:

| Property | Description |
|----------|-------------|
| **Label** | Text shown on the wedge. Keep short. |
| **Icon** | Optional Slate SVG icon drawn beside the label. Select via the icon picker. |
| **Background Tint** | Overrides the wedge background color. Unset = use theme default. |
| **Bold** | Renders the label in bold. |

### Editor Command
Executes a registered editor command by context and name.

- **Command Context** - binding context (e.g. `LevelEditor`)
- **Command Name** - command key within that context (e.g. `PlayInViewport`)

Use the command picker dropdown to browse and search all registered commands.

### Console Command
Passes a string to `GEngine->Exec` against the editor world.

- **Command** - any console command (e.g. `stat fps`, `viewmode lit`)

### Editor Utility Widget
Opens an Editor Utility Widget Blueprint as a tab.

- **Widget** - soft reference to the widget blueprint asset

### Editor Utility Object
Instantiates an Editor Utility Object blueprint and calls its `Run` event.

- **Object** - soft reference to the Editor Utility Object Blueprint asset

### Category
Groups child items into a nested ring. In hold mode, hovering the wedge for `Category Hover Ms` auto-navigates into it. In tap mode, left-clicking navigates immediately.

- **Children** - nested array of any item types, including further categories

### Close
Closes the current level of the menu. In a sub-ring: navigates back to the parent ring. At root: closes the menu entirely. Place it anywhere in a `Children` array to control its wedge position. Label and icon are fully customizable.
