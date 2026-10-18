# Riot defection preserves the partial-Hurry marker

Partial-Hurry is Base status mask **0x40000000**. The actual riot-defection status cleanup reads the whole word, ANDs it with **0xffdef7b1**, and writes the whole word back. Since the mask retains0x40000000, this specific cleanup **preserves** the partial-Hurry marker. This is riot-source evidence, independent of capture's separately measured preservation.

Keep the full status word, including opaque retained bits. The cleanup does not imply that later production/upkeep can never clear partial-Hurry; its separate normal lifecycle still applies. This is a field-only source mapping, not a new full-turn/grid run.
