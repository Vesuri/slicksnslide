# Integration checkpoint — 2026-09-25

Evidence record, not a work queue. Current actions are in `open-work.md`.

The accumulated native implementation and verification tools were committed
as `eb2ba00`; emulator/debug fixtures as `de39b0a`. These are present-day
integration checkpoints, not reconstructed or backdated development history.

## Checks run before committing

The following targets completed successfully in one regression run:

```text
verify-saved-game-resume
verify-saved-game-storage
verify-saved-game
verify-amiga-audio-volume
verify-audio-pitch
verify-configuration
verify-player-profiles
verify-setup-storage
verify-setup-load
verify-profile-actions
verify-race-options
verify-dirty-tracking
verify-amiga-joystick
verify-amiga-key-scan
```

The Amiga build, runtime strlen guard and syntax checks for `amiga/*.sh`
also passed. This is a selected integration suite, not a claim that every
historical diagnostic or every Make target was rerun. Original-code tests
consume local ignored runtime/assets.

Source whitespace checks passed. Stored DOSBox patch files were separately
parsed with `git apply --numstat`: their context-prefix space followed by a
source tab triggers Git's ordinary space-before-tab warning and was preserved
as patch syntax, not rewritten as source indentation.

No new emulator run was required solely for documentation/commit cleanup.
Recent native audio and setup gates, including their limitations, are recorded
in `audio-channel-plan.md` and `player-setup-completion.md`. Manual joystick
input remains unverified and explicitly deferred by the user.

## Repository checks

Author and committer resolve to Vesa Halttunen <vesuri@jormas.com>. Repository
hooks point to `/dev/null`; commit/tag/push signing is disabled. Commit calls
also explicitly disabled hooks and signing. No co-author trailers were added.
Only source, tools, fixtures and documentation were staged; original game
data, generated source/reference bytes, captures, builds and machine-specific
emulator state remain ignored. These commits were not pushed.
