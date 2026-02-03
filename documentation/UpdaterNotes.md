# Updater Notes

## Shutdown trigger
- Send a standard `WM_CLOSE` to the main window (top-level AMS window).
- The message is routed to the normal close path (same as clicking the window close button).

## Confirmation signal
- Watch the application log for the machine-readable line:
  - `PROCESS_EXIT_REACHED`
- This line is emitted when the process is exiting, including forced-quit paths.

## Expected timing
- Normal shutdown: diagnostics are logged immediately, and the Qt event loop exits shortly after.
- Forced quit (when `AMS_FORCE_QUIT_ON_SHUTDOWN` is set): diagnostics are logged, then a forced exit is triggered after the 10-second watchdog window.
