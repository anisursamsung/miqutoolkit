# Miqutoolkit

A lightweight C++20 UI framework for Wayland layer-shell applications, built with Cairo and Pango.

## Features

- **Wayland Native**: Direct `wlr-layer-shell` integration for overlays, panels, and modals.
- **Declarative Builders**: `WindowBuilder`, `CardViewBuilder`, `LinearLayoutBuilder`, `GridViewBuilder`, `SearchViewBuilder`.
- **Widgets**: `CardView`, `TextView`, `EditText`, `ImageView` (SVG/PNG with system icon lookup), `Button`, `GridView`.
- **Dynamic Theming**: `ColorScheme` automatically reads colors and metrics from `miquland.conf`.

## Dependencies

Arch Linux:
```bash
sudo pacman -S --needed \
    base-devel cmake pkgconf git \
    wayland wayland-protocols libxkbcommon \
    cairo pango librsvg libpng
```

## Build & Install

```bash
cd miqutoolkit
# Build locally
./make.sh

# Install system-wide to /usr
sudo ./make.sh
```

Installs:
- Headers: `/usr/include/miqutoolkit/`
- Library: `/usr/lib/libmiqutoolkit.so`
- Pkg-config: `/usr/lib/pkgconfig/miqutoolkit.pc`

## Example

```cpp
#include <miqutoolkit/miqutoolkit.hpp>

using namespace miqu;

int main(int argc, char* argv[]) {
    auto engine = AppEngine::create();
    auto theme = ColorScheme::get();

    auto search = SearchViewBuilder::create()
        ->title("Search")
        ->hint("Type to filter...")
        ->focused(true)
        ->padding(16, 12)
        ->build();

    auto card = CardViewBuilder::create()
        ->backgroundColor(theme->colors.surface)
        ->stroke(1, theme->colors.outline)
        ->cornerRadius(theme->metrics.corner_radius)
        ->padding(16)
        ->addView(search)
        ->build();

    auto window = WindowBuilder::create()
        ->role(WindowRole::LayerOverlay)
        ->keyboardInteractive(true)
        ->contentSize(600, 300)
        ->contentView(card)
        ->onClose([&]() { engine->quit(); })
        ->build();

    return engine->enter_loop();
}
```

Compile with:
```bash
g++ -std=c++20 main.cpp $(pkg-config --cflags --libs miqutoolkit) -o app
```
