# SimHub Plugin Profile Guide

Profiles let you bundle pedal hardware configs, basic effect toggles (`ABS` / `TC`, `B.P`), and unlimited effect scripts (`*.js` / `*.ncalc`) into a single preset that can be applied manually or switched automatically by game or car.

Profile files are stored as JSON in:
`SimHub\PluginsData\Common\DiyFfbPedal\profiles\`

---

## 1. Managing Profiles (Profile List)

Right-click inside the **Profile List** (or on a specific profile item) to open the context menu:

- **Add new Profile**: Creates a new empty profile JSON file.
- **Read**: Loads the selected profile into the editor on the right so you can inspect or modify it (does not apply it to the pedals).
- **Apply**: Applies the selected profile to the pedals immediately (sends configs, updates `ABS`/`TC`/`B.P`, and syncs bound scripts).
- **Overwrite**: Saves your current edits on the right-hand panel back to the selected profile file.
- **Save as New Profile**: Saves the current editor state as a new profile file.
- **Rename**: Renames the selected profile file.
- **Refresh List**: Re-scans the `profiles\` folder for externally added or edited files.
- **Open folder**: Opens `PluginsData\Common\DiyFfbPedal\profiles\` in Windows Explorer.
- **Delete**: Deletes the selected profile file.

---

## 2. Game / Car Auto-Switch Binding

- **Game or Car ID**: Enter a single SimHub game code or car ID (exact text match, case-insensitive).
- **Current Game**: Fills the field with the game code currently detected by SimHub.
- **Current Car**: Fills the field with the car ID currently detected by SimHub.
- **Clear**: Clears the binding field.

**Matching priority**: When a game session starts, the plugin checks for a matching **Car ID** first. If no profile matches the car ID, it falls back to matching the **Game** code.

---

## 3. Pedal Configs

For each pedal (**Clutch**, **Brake**, **Throttle**):
- Select a saved pedal config from the dropdown (`PluginsData\Common\DiyFfbPedal\configs\`), or click **Clear** to leave that pedal unchanged when the profile is applied.

---

## 4. Effects and `Scripts (N)`

Under each pedal's config dropdown:
- **ABS / TC** and **B.P**: Checkboxes to enable or disable those built-in effects when the profile is applied.
- **Scripts (N)**: Opens the **Profile Scripts** window for that pedal, where `N` is the number of bound scripts.
  1. Pick a script from the dropdown (loaded from `PluginsData\Common\DiyFfbPedal\effects\`) and click **+ Add**. You can bind any number of scripts per pedal, and the same `.js` / `.ncalc` file can be shared across Clutch, Brake, and Throttle.
  2. Use the checkbox to set **Enabled**, adjust the per-profile **Gain**, or click **Remove** to unbind it.
  3. Click **OK**, then **Overwrite** (or **Save as New Profile**) on the profile to save the bindings.

> **Important**: For safety, effect scripts will only run on the pedals if the safety acknowledgment checkbox in the **Pedal > Effects > Scripts** tab is checked.

---

## 5. Applying Profiles

- **Automatic**: When SimHub detects a matching **Car ID** or **Game**, the profile is applied automatically.
- **Manual**: Right-click a profile and choose **Apply**, use the **Quick** buttons on the **Home** tab, or bind SimHub actions/shortcuts to profiles in **System Settings > Shortcuts**.
