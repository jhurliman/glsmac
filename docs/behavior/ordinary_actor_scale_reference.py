"""4d8400 entry multiplier, before body remapping and native-design modifiers.

Stored byte component IDs are inputs. Constants are original binary32 values;
the ordinary view stores its base before this separate multiplication/store.
This helper does not claim the later design8/9/10 native-creature scale factors.
"""
import struct


def multiplier(chassis,weapon):
    assert 0<=chassis<=255 and 0<=weapon<=255
    if chassis==0:
        return 1.25 if weapon<3 or weapon==18 else struct.unpack('<f',struct.pack('<I',0x3f4ccccd))[0]
    return 1.25 if chassis==1 and weapon==18 else 1.0
