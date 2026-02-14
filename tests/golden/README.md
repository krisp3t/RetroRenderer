# Golden Hash Baselines

This folder contains committed framebuffer hash baselines used by `GoldenRenderingTests.cpp`.

Baselines support:

1. Platform-specific keys: `name.windows`, `name.linux`, `name.macos`
2. Backward-compatible generic keys: `name`
3. Windows enforcement is opt-in by default: set `RETRO_RUN_GOLDEN_WINDOWS=1`

## Updating baselines

Only update these when visual output changes intentionally.

1. Rebuild tests.
2. Run tests with `RETRO_UPDATE_GOLDENS=1`.
   This writes platform-specific keys for the current OS.
3. Re-run tests without `RETRO_UPDATE_GOLDENS` and ensure they pass.
4. Review and commit the updated `software_pipeline_hashes.txt`.
