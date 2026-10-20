"""Flags16 mutation rules; record import and status remain distinct fields."""
DISCLOSURE_CLEAR_MASK=0xfbbf
def disclose(word,observer):return word&DISCLOSURE_CLEAR_MASK if observer else word
def editor_attributes(word,selection):
 result=word&0xf81f
 for option,flag in [(1,0x20),(2,0x80),(4,0x100),(8,0x200),(0x10,0x40),(0x20,0x400)]:
  if selection&option:result|=flag
 return result
def upkeep(word,duration):
 if not duration:return word,0
 duration-=1
 return (word if duration else word&DISCLOSURE_CLEAR_MASK),duration
def legacy48_to52(old):
 """Original version<3 Unit record translation, including exact flag bytes."""
 if len(old)!=48:raise ValueError('Expected48 bytes')
 result=bytearray(52);result[:10]=old[:10];result[10]=old[10];result[14]=old[11];result[16:20]=old[12:16];result[20:28]=old[16:24];result[28:36]=old[24:32];result[36:46]=old[32:42];result[46:52]=old[42:48]
 return bytes(result)
