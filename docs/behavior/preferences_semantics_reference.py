"""Self-authored encoder/decoder for the67 created original preferences."""

import json
from pathlib import Path

CONTRACT = json.loads(Path(__file__).with_name(
    "preferences-semantics-cases.json").read_text())
CONTROLS = CONTRACT["controls"]
ACCEPTED_MASKS = tuple(CONTRACT["accepted_masks"])


def decode(words):
    checked = {c["id"]: bool(words[c["word"]] & c["mask"]) ^ c["inverted"]
               for c in CONTROLS}
    audio = {c["role"]: bool(words[c["word"]] & c["mask"])
             for c in CONTRACT["audio_controls"]}
    return checked, audio


def encode(checked, audio):
    """Build fresh words, as the accepted UI path does; preserve no old bits."""
    words = [0, 0, 0]
    for c in CONTROLS:
        if bool(checked[c["id"]]) ^ c["inverted"]:
            words[c["word"]] |= c["mask"]
    for c in CONTRACT["audio_controls"]:
        if audio[c["role"]]:
            words[c["word"]] |= c["mask"]
    return tuple(words)


def default_words(laptop=0, difficulty=0, restore_button=False):
    """The reset helpers omit the loader's low-difficulty tutorial OR20."""
    word0 = 0xA3E1DD16 if laptop else 0xBBE1DD96
    if not restore_button and difficulty < 2:
        word0 |= 0x20
    return word0, (0x00327168 if laptop else 0x003A7168), 0x0003C3A9
