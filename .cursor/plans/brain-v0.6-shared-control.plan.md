# Brain v0.6.0 shared PS4 control overlay

Treat `BRAIN_v0.5.0_to_v0.6.0.patch` as an additive overlay on Brain v0.5.

## Overlay

- Manual PS4 input = temporary override (Brain stays armed; resumes when sticks neutral)
- OPTIONS opens autonomy STOP + runtime menu (Brain / Imitation / Sense React / Free Roam / Stop)
- `shared_control.h`, `autonomy_menu.*`, `user_mode_protocol.h`, `U,MODE,...`
- Camera/SD from v0.5 unchanged

## Kept safety

BRAKE / `ra_link::brake()`, 500 cm radar, `brainForwardIsSafe`, poll/tick, creepBack, visual memory, imitation, secrets untouched.

## Verify

- native 152/152 (incl. `test_brain_v06`)
- `src` / `esp32_r4` / `esp32_fisheye` SUCCESS
