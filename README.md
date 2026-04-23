# PiUE - Pie Menu for Unreal Editor

Blender-style radial quick-action menu for the Unreal Engine level editor viewport.

![Editor Screenshot](Resources/Screenshot.png)

### Installation
Get `PiUE.zip` from the [releases](https://github.com/Solessfir/PiUE/releases) and extract it into your project's `Plugins` folder.

## Usage

Press **V** (or **Mouse 4**) while the level viewport has focus to open the menu. **Tap** (< `TapThreshold`) leaves the menu open - click a wedge or press again to close. **Hold** (≥ `TapThreshold`) executes the highlighted wedge on release. Move the cursor away from center to highlight a wedge; stay in the dead zone to close without acting.

The menu is unavailable while Play In Editor is active.

> Both bindings can be rebound in **Editor Preferences → Keyboard Shortcuts → PiUE → Summon PiUE Radial Menu**.

## Configuration

**Editor Preferences → Plugins → PiUE**

### Menu

| Property | Description |
|----------|-------------|
| **Menu Items** | Root-level actions and categories. Add via the `+` button. |

### Input

| Property | Default | Description |
|----------|---------|-------------|
| **Tap Threshold** | `150` | Short press leaves the menu open for click navigation. Long press executes the hovered wedge on release. |
| **Category Hover Ms** | `1000` | How long (ms) a category wedge must be hovered before auto-navigating into it. |

### Layout

| Property | Default | Description |
|----------|---------|-------------|
| **Menu Radius** | `120` | Ring radius in screen pixels. |
| **Dead Zone Radius** | `25` | Cursor distance from center below which nothing is selected. |

### Animation

| Property | Default | Description |
|----------|---------|-------------|
| **Wedge Anim Speed** | `25` | Speed multiplier for wedge enter/exit animation. Higher = snappier. |

### Style

| Property | Description |
|----------|-------------|
| **Default Wedge Tint** | Background color for unselected wedges. |
| **Highlight Wedge Tint** | Background color for the hovered wedge. |

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
Groups child items into a nested ring. Hovering the wedge for `Category Hover Ms` navigates into the category.

- **Children** - nested array of any item types, including further categories

### Close
Closes the current level of the menu. In a sub-ring: navigates back to the parent ring. At root: closes the menu entirely. Place it anywhere in a `Children` array to control its wedge position. Label and icon are fully customizable.
