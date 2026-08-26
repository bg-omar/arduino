# Changelog — Wall-Z Brain v0.6.0

- Changed PS4/Brain arbitration from **manual input = disarm** to **manual input = temporary override**.
- Brain remains armed while PS4 drive/head input is active.
- Brain resumes automatically when PS4 input becomes neutral.
- Removed manual-input rejection from Brain ARM; active RA robot modes still block Brain ARM.
- Added explicit manual-override pause/resume acknowledgements.
- Reassigned `OPTIONS` from direct Sense React toggle to an autonomy STOP + runtime mode menu.
- Added runtime choices: Brain Auto, Brain Imitation, Sense React, Free Roam, Stop/Manual.
- Kept `TOUCHPAD` as the original persistent SETUP.TXT/config menu.
- Added RA->S3 `U,MODE,...` mode-request protocol so the physical PS4 menu and S3 Brain state stay synchronized.
- Added native shared-control arbitration tests.
- Camera/local-SD architecture from v0.5 remains unchanged.
