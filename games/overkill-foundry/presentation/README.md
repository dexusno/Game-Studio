# Precision input, first implementation

`precision.hpp` is a small presentation controller, shared by the Slate widget
and its isolated input tests. It knows only elapsed focused time and returns a
Miss/Good/Perfect category. It knows no materials, upgrade ownership, rewards or
RNG state. The production caller supplies the core's current
`upgradePrecisionWidthPercent` and submits the resulting category through the
ordinary Collect or saved Precision-retry command.

Initial control tuning is `precision-ui-0.1`: one 1.8-second sweep, centred Good
band of 32% of the track and Perfect band of 9%. The widths are multiplied by
the explicit core-provided percentage. These are implementation values for
human testing, not owner-approved feel or balance results.

Before Start, Back leaves the game unchanged. Once started, one Space/Enter or
Stop click resolves the attempt; holding the key does not repeat it. Timeout
records Miss. Escape cannot cancel an already moving marker. Focus loss pauses
presentation time and submits nothing; the first frame back discards time spent
outside the game. Quitting and Continue retain the ordinary whole-fight restart
contract. An upgrade retry uses its existing saved choice; this widget grants
no additional attempts. The core remains responsible for ordinary haul safety,
bonus quantities, conditional recipe effects and once-per-fight enforcement.

Forty-six native assertions pass with MSVC 19.44, C++17, `/W4 /WX /permissive-`:
before-start safety, duplicate start/stop, timeout, focus interruption, all band
boundaries at 100/120/130% width and invalid frame-time rejection. Build the
standalone target in `unreal/Tools/precision-tests` with CMake and run CTest.
This does not establish Slate focus, real input, claw/camera integration,
enjoyment or owner acceptance; those require the graphical candidate.
