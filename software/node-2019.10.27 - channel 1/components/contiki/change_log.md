# The changed log for contiki 3.0 port to RT-Thread by armink

- Add `MAC_ACK_WAIT` for nullrdc ACK wait
- Add `SERIAL_LINE_IGNORE_CHAR` and `SERIAL_LINE_END_CHAR` for serial line
- Change serial-shell.c for serial-line arch port
- Rename `shell.c` to `shell-core.c` on `apps/shell`
- Rename `cmd.c` to `slip-cmd.c` on `apps/slip-cmd`
- Rename `timer.c` to `timer-core.c` on `core/sys`
