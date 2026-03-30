# Visual Studio 2026 – UI Shell Background Colors

Default background colors for the main UI elements in the **Dark** and **Light** base themes (official defaults as of March 2026).

### Primary Background Colors – Dark Theme (Base)

| UI Element                          | Hex Value     | Approximate RGB    | Common Usage |
|-------------------------------------|---------------|--------------------|--------------|
| Main IDE / Environment Background   | `#FF2D2D30`   | (45, 45, 48)       | Overall frame, auto-hide tabs, some command bars |
| Tool Window Background              | `#FF252526`   | (37, 37, 38)       | Docked tool windows (Solution Explorer, Properties, Output, etc.) |
| Document / Editor Window Background | `#FF252526`   | (37, 37, 38)       | Most content areas; code editor Plain Text default is often `#FF1E1E1E` |
| Menu Bar Background                 | `#FF2D2D30`   | (45, 45, 48)       | Top menu bar |
| Toolbar / Command Bar Background    | `#FF2D2D30`   | (45, 45, 48)       | Toolbars, ribbon-style bars |
| Status Bar Background               | `#FF007ACC`   | (0, 122, 204)      | Accent blue (same in both themes) |
| Inactive Tab / Dock Target          | `#FF252526`   | (37, 37, 38)       | Unselected tabs, docking glyphs |
| Dropdown Menu / Popup Background    | `#FF1B1B1C`   | (27, 27, 28)       | Menu dropdowns, context menus |
| Divider Lines / Borders             | `#FF2D2D30`   | (45, 45, 48)       | Separators, grid lines |

### Primary Background Colors – Light Theme (Base)

| UI Element                          | Hex Value     | Approximate RGB    | Common Usage |
|-------------------------------------|---------------|--------------------|--------------|
| Main IDE / Environment Background   | `#FFEEEEF2`   | (238, 238, 242)    | Overall frame, auto-hide tabs |
| Tool Window Background              | `#FFF5F5F5`   | (245, 245, 245)    | Docked tool windows |
| Document / Editor Window Background | `#FFF5F5F5`   | (245, 245, 245)    | Most content areas |
| Menu Bar Background                 | `#FFEEEEF2`   | (238, 238, 242)    | Top menu bar |
| Toolbar / Command Bar Background    | `#FFEEEEF2`   | (238, 238, 242)    | Toolbars |
| Status Bar Background               | `#FF007ACC`   | (0, 122, 204)      | Accent blue |
| Inactive Tab / Dock Target          | `#FFF5F5F5`   | (245, 245, 245)    | Unselected tabs |
| Dropdown Menu / Popup Background    | `#FFF6F6F6`   | (246, 246, 246)    | Menu dropdowns |
| Divider Lines / Borders             | `#FFEEEEF2`   | (238, 238, 242)    | Separators |

**Notes**  
- These values are the official defaults for Visual Studio 2026 and form the foundation for all tinted themes (Cool Breeze, Moonlight Glow, etc.).  
- You can verify them live in the IDE using the free **Color Picker Tool** extension.  
- For hover states, borders, or syntax colors, see **Tools > Options > Environment > Fonts and Colors**.