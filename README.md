# 🛠️ Miqutoolkit

A declarative, lightweight, modern **C++20 UI framework** designed for Wayland overlay and desktop applications, built on top of **Cairo**, **Pango**, **libwayland**, and **wlr-layer-shell**.

---

## ✨ Features

- **Declarative Builder API:** Intuitive Android/Flutter-like declarative builder pattern (`LinearLayoutBuilder`, `CardViewBuilder`, `GridViewBuilder`, `SearchViewBuilder`, `WindowBuilder`).
- **Native Wayland Layer Shell Support:** Seamless overlay, panel, and popup creation with exclusive zones, keyboard interactivity, and anchor controls (`wlr-layer-shell-unstable-v1`).
- **Dynamic Theming Engine:** Built-in `ColorScheme` engine with automatic parsing of Miquland configuration (`miquland.conf`, `theme_mode.conf`, `light.conf`, `dark.conf`).
- **Rich Widget Library:**
  - **`GridView`:** High-performance responsive grid with auto-fitting columns, keyboard navigation, smooth scrolling, and live query filtering.
  - **`SearchView`:** Rounded pill input widget with label support and text change listeners.
  - **`CardView` / `FrameLayout` / `LinearLayout`:** Flexible container hierarchy supporting padding, margins, layout weights, and rounded corners.
  - **`TextView` / `EditText`:** Full Unicode and Pango text rendering, cursor handling, and input processing.
  - **`ImageView`:** Automatic system icon theme resolution (Papirus, Tela-circle, hicolor, pixmaps) with SVG and PNG caching.
- **Desktop Application Scanner:** Built-in `PackageManager` for asynchronous desktop file (`.desktop`) discovery, icon extraction, and application launching.
- **Zero Heavy Dependencies:** Fast startup (<1ms) without massive framework overhead like Qt or heavy WebViews.

---

## 📦 Installation & Dependencies

### Arch Linux Dependencies
```bash
sudo pacman -S --needed \
    base-devel cmake pkgconf git \
    wayland wayland-protocols libxkbcommon \
    cairo pango librsvg libpng
```

### Build & Install to System (`/usr`)
```bash
sudo ./make.sh
```

`make.sh` installs:
- Headers: `/usr/include/miqutoolkit/`
- Shared Library: `/usr/lib/libmiqutoolkit.so`
- Pkg-config file: `/usr/lib/pkgconfig/miqutoolkit.pc`

---

## 💻 Quickstart Example

Here is how simple it is to create a Wayland overlay window with Miqutoolkit:

```cpp
#include <miqutoolkit/miqutoolkit.hpp>

using namespace miqu;

int main(int argc, char* argv[]) {
    // 1. Create Engine & Theme
    auto engine = AppEngine::create();
    auto theme = ColorScheme::get();

    // 2. Build Search Input
    auto search = SearchViewBuilder::create()
        ->title("Search")
        ->hint("Type to filter...")
        ->focused(true)
        ->padding(16, 12)
        ->build();

    // 3. Root Card Container
    auto rootCard = CardViewBuilder::create()
        ->backgroundColor(theme->colors.surface)
        ->stroke(1, theme->colors.outline)
        ->cornerRadius(theme->metrics.corner_radius)
        ->padding(20)
        ->addView(search)
        ->build();

    // 4. Create Layer Shell Overlay Window
    auto window = WindowBuilder::create()
        ->role(WindowRole::LayerOverlay)
        ->keyboardInteractive(true)
        ->dimBackdrop(true)
        ->contentSize(600, 300)
        ->contentView(rootCard)
        ->onClose([&]() { engine->quit(); })
        ->build();

    // 5. Enter Wayland Event Loop
    return engine->enter_loop();
}
```

### Compiling Your App with `pkg-config`
```bash
g++ -std=c++20 main.cpp $(pkg-config --cflags --libs miqutoolkit) -o my_app
```

---

## 🏛️ Architecture Overview

```
miqutoolkit/
├── core/
│   ├── app_engine       # Wayland display loop & surface manager
│   ├── color            # Color parsing & blending
│   ├── color_scheme     # Theme parser (Material Dark & Light)
│   ├── shm_pool         # Shared memory double-buffering
│   └── window           # Layer-shell window abstraction
├── view/
│   ├── view             # Base widget & event routing
│   ├── card_view        # Rounded cards & containers
│   ├── linear_layout    # Flexbox-style linear layouts
│   ├── grid_view        # Auto-fit scrollable application grid
│   ├── search_view      # Live search box
│   ├── edit_text        # Interactive text editor
│   ├── text_view        # Pango text display
│   └── image_view       # SVG / PNG icon renderer
└── system/
    └── package_manager  # XDG .desktop parsing & app launcher
```

---

## 📜 License

MIT License. Developed as part of the Miquland Desktop Ecosystem.
