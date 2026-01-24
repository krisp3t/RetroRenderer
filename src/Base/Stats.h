#pragma once

namespace RetroRenderer {
struct Stats {
    int renderedTris = 0;
    int renderedVerts = 0;

    void Reset() {
        renderedTris = 0;
        renderedVerts = 0;
    }
};
} // namespace RetroRenderer