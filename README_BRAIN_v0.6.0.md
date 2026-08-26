# Wall-Z Brain v0.6.0 — Shared PS4 Control + Runtime Autonomy Menu

## Authority model

The PS4 controller may remain connected permanently. Connection state does **not** disarm Brain.
Only real non-neutral PS4 drive/head input temporarily owns the actuators:

```
PS4 input active  -> PS4 owns motors/head; Brain remains armed but paused
PS4 input neutral -> Brain resumes automatically on the first neutral telemetry frame
OPTIONS           -> explicit autonomy STOP + runtime autonomy menu
```

RA4M1 safety remains final authority for sonar, heartbeat and bounded move pulses.

## OPTIONS runtime menu

`OPTIONS` first stops every autonomous actuator source, then opens:

1. Brain Auto
2. Brain Imitation
3. Sense React
4. Free Roam
5. Stop / Manual

Controls:

- D-pad Up/Down: select
- Cross/X: activate
- Circle: close menu and remain stopped/manual
- OPTIONS: close/open; autonomy is stopped before the menu is opened
- TOUCHPAD: original SETUP.TXT/configuration menu (unchanged)

## Boot behavior

v0.6 keeps the conservative boot default: Brain observes/learns, but autonomous motion is not automatically armed after reset. Select `Brain Auto` or `Brain Imitation` from OPTIONS, or ARM from the web UI.

## Shared control behavior

An armed Brain is no longer disarmed by `PS4::isManualControlActive()`.
The RA4M1 cancels any outstanding Brain movement pulse **without issuing Motor::Car_Stop()**, because the PS4 command may already have been applied. While manual input remains active, incoming Brain MOVE/HEAD commands are rejected. When sticks return neutral, the S3 immediately resumes Brain action scheduling.

Existing RA robot modes remain mutually exclusive with Brain autonomy. Selecting Sense React or Free Roam disarms Brain and gives RA4M1 mode code actuator authority; PS4 manual input still temporarily overrides those existing modes as before.
