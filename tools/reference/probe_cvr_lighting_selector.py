"""Execute a bounded SMAC lighting-selector suffix with explicit x87 angles.

The preceding inverse/vector/trigonometric work does not execute. The original
table bytes and generated reports must remain outside Git.
"""
from collections import Counter
from fractions import Fraction
from pathlib import Path
import argparse
import hashlib
import json
import struct

import capstone
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
from unicorn.x86_const import (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_FP0, UC_X86_REG_FP1, UC_X86_REG_EIP)
from probe_scout_composition import Probe as Image, EXE_HASH, require, words, within
import probe_cvr_pixels as pixels

CODE = [(0x65ac62, 0x65aca6), (0x660678, 0x66069f)]
CONSTANTS = [(0x695064, 0x695068), (0x695070, 0x695078)]
CONSTANT_BITS = (0x3f000000, 0xc16a46ad, 0xc162a2fa)
ROWS, TABLE, OUTPUT = 0x6c2e88, 0x6b6588, 0x9c6b94
ROW_SIZE, ROW_COUNT, WINDOW_SIZE = 2144, 24, 2048
TABLE_HASH = '81102740e2d482516c9d1b42fec5972c2ba8364cb597fd7492e82d1e287eec60'
STACK, STOP, ESP = 0x70000000, 0x71000000, 0x7000ff00
SAVED = {UC_X86_REG_EBX: 0x11111111, UC_X86_REG_ESI: 0x22222222,
         UC_X86_REG_EDI: 0x33333333, UC_X86_REG_EBP: 0x44444444}


def sha(value):
    return hashlib.sha256(value).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def exact(value):
    return Fraction.from_float(struct.unpack('<f', words(value))[0])


def exact_extended(value):
    numerator = abs(value.numerator)
    while numerator and not numerator & 1:
        numerator >>= 1
    require(numerator.bit_length() <= 64, 'domain: intermediate exceeds exact extended significand')


def raw80(value):
    """Exact raw x87 encoding of an admitted nonnegative normal binary32/zero."""
    if value == 0:
        return (0, 0)
    exponent = (value >> 23) & 255
    require(0 < exponent < 255 and not value >> 31, 'domain: unsupported x87 seed')
    return (((value & 0x7fffff) | 0x800000) << 40, exponent - 127 + 16383)


def oracle(angle_bits, mutation=None):
    require(len(angle_bits) == 2, 'domain: exactly two binary32 angle words required')
    rounded = []
    for value, constant, maximum in zip(angle_bits, CONSTANT_BITS[1:], (23, 89)):
        require(isinstance(value, int) and 0 <= value <= 0xffffffff, 'domain: invalid binary32 word')
        require(not value >> 31, 'domain: negative angle/sign bit')
        exponent = (value >> 23) & 255
        require(exponent != 255, 'domain: nonfinite angle')
        require(value == 0 or exponent != 0, 'domain: subnormal angle excluded')
        product = exact(value) * -exact(constant)
        shifted = product + Fraction(1, 2)
        exact_extended(product)
        exact_extended(shifted)
        result = int(product if mutation == 'omit-half' else shifted)
        require(0 <= result <= maximum, 'domain: rounded selector index outside supported row/bin range')
        rounded.append(result)
    row, azimuth = rounded
    wrapped = azimuth if mutation == 'omit-wrap' or azimuth != 89 else 0
    offset = (88 if mutation == 'offset-88' else 89) - wrapped
    pointer = TABLE + row * ROW_SIZE + offset
    if mutation == 'wrong-row':
        pointer += ROW_SIZE
    return rounded, offset, pointer


class LightingProbe:
    def __init__(self, executable):
        self.image = Image(executable)
        require(b''.join(self.image.read_disk(a, b - a) for a, b in CONSTANTS) == words(*CONSTANT_BITS),
                'original selector constant bits changed')
        self.row_pointers = [TABLE + i * ROW_SIZE for i in range(ROW_COUNT)]
        require(self.image.read_disk(ROWS, ROW_COUNT * 4) == words(*self.row_pointers), 'original row pointers changed')
        self.table = self.image.read_disk(TABLE, ROW_COUNT * ROW_SIZE)
        require(sha(self.table) == TABLE_HASH and min(self.table) == 0 and max(self.table) == 23,
                'original table fingerprint/value range changed')
        dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.instructions = {}
        for start, end in CODE:
            decoded = list(dis.disasm(self.image.read_disk(start, end - start), start))
            require(decoded and decoded[-1].address + decoded[-1].size == end, 'range ends mid-instruction')
            self.instructions.update({ins.address: ins for ins in decoded})

    def run(self, name, angle_bits, control_word=0x037f, deny=None, mutation=None, budget=150):
        require(control_word == 0x037f, 'domain: requires nearest-even extended x87 control word')
        rounded, offset, pointer = oracle(angle_bits, mutation)
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        image = self.image
        uc.mem_map(image.base, (image.size + 4095) & ~4095)
        uc.mem_write(image.base, image.raw[:image.header])
        for start, _, length, disk_offset in image.sections:
            uc.mem_write(start, image.raw[disk_offset:disk_offset + length])
        uc.mem_map(STACK, 0x10000)
        uc.mem_map(STOP, 0x1000)
        uc.mem_write(STACK, b'\xa5' * 0x10000)
        # This is a suffix, after the original function's sub esp,16; push esi.
        uc.mem_write(ESP, words(SAVED[UC_X86_REG_ESI]))
        uc.mem_write(ESP + 20, words(STOP))
        uc.mem_write(OUTPUT - 16, b'\x5a' * 36)
        before_stack = bytes(uc.mem_read(STACK, 0x10000))
        stack_reads = [(ESP - 20, ESP + 24)]
        stack_writes = [(ESP - 20, ESP - 8), (ESP - 8, ESP)]
        reads = stack_reads + CONSTANTS + [(ROWS, ROWS + ROW_COUNT * 4)]
        writes = stack_writes + [(OUTPUT, OUTPUT + 4)]
        if deny == 'row-read':
            reads.remove((ROWS, ROWS + ROW_COUNT * 4))
        if deny == 'constant-read':
            reads.remove(CONSTANTS[1])
        if deny == 'output-write':
            writes.remove((OUTPUT, OUTPUT + 4))
        trace, execution, count, finished = [], Counter(), 0, False

        def code_hook(machine, address, size, _):
            nonlocal count, finished
            if address == STOP:
                finished = True
                machine.emu_stop()
                return
            require(address in self.instructions and within(address, size, CODE), f'unexpected code {address:#x}')
            require(deny != 'helper-code' or address != CODE[1][0], 'control denied conversion helper')
            count += 1
            require(count <= budget, 'instruction budget exceeded')
            execution[address] += 1

        def memory_hook(machine, access, address, size, value, _):
            read = access == UC_MEM_READ
            require(within(address, size, reads if read else writes),
                    f'unexpected {"read" if read else "write"} {address:#x}+{size}')
            data = bytes(machine.mem_read(address, size)) if read else (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
            trace.append({'instruction': hex(machine.reg_read(UC_X86_REG_EIP)), 'kind': 'read' if read else 'write',
                          'address': hex(address), 'size': size, 'bytes': data.hex()})

        uc.hook_add(UC_HOOK_CODE, code_hook)
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_hook)
        for register, value in SAVED.items():
            uc.reg_write(register, value)
        uc.reg_write(UC_X86_REG_ESI, 0x55555555)  # Current scratch differs from the saved callee value.
        uc.reg_write(UC_X86_REG_ESP, ESP)
        uc.reg_write(UC_X86_REG_EFLAGS, 2)
        uc.reg_write(UC_X86_REG_FPCW, control_word)
        uc.reg_write(UC_X86_REG_FPSW, 0)
        # TOP=0; ST0=azimuth, ST1=inclination. No fixture instructions execute.
        seeds = [raw80(angle_bits[1]), raw80(angle_bits[0])]
        tags = 0xfff0 | int(angle_bits[1] == 0) | (int(angle_bits[0] == 0) << 2)
        uc.reg_write(UC_X86_REG_FP0, seeds[0])
        uc.reg_write(UC_X86_REG_FP1, seeds[1])
        uc.reg_write(UC_X86_REG_FPTAG, tags)
        require([uc.reg_read(UC_X86_REG_FP0), uc.reg_read(UC_X86_REG_FP1)] == seeds, 'x87 raw seed mismatch')
        uc.emu_start(CODE[0][0], STOP + 1, timeout=2000000, count=budget + 1)
        require(finished, 'did not reach sentinel before budget/timeout')
        require(uc.reg_read(UC_X86_REG_ESP) == ESP + 24, 'suffix stack cleanup changed')
        require(all(uc.reg_read(reg) == value for reg, value in SAVED.items()), 'callee-saved register changed')
        require(uc.reg_read(UC_X86_REG_FPCW) == control_word, 'x87 control word not restored')
        status = uc.reg_read(UC_X86_REG_FPSW)
        require(status & 0x3800 == 0x1000 and uc.reg_read(UC_X86_REG_FPTAG) == 0xffff,
                'x87 stack not empty with two popped seeds')
        require(status & 0x1f == 0, 'unexpected x87 invalid/denormal/divide/overflow/underflow status')
        require(execution[CODE[1][0]] == 2, 'helper call count changed')
        require(bytes(uc.mem_read(OUTPUT - 16, 36)) == b'\x5a' * 16 + words(pointer) + b'\x5a' * 16,
                'selector pointer/canary oracle mismatch')
        captured_pointer = struct.unpack('<I', uc.mem_read(OUTPUT, 4))[0]
        helper_values = [struct.unpack('<q', bytes.fromhex(t['bytes']))[0] for t in trace
                         if t['kind'] == 'write' and int(t['address'], 16) == ESP - 20 and t['size'] == 8]
        require(helper_values == rounded, 'original conversion qword mismatch')
        control_writes = [(int(t['address'], 16), t['bytes']) for t in trace
                          if t['kind'] == 'write' and int(t['address'], 16) in (ESP - 10, ESP - 12)]
        require(control_writes == [(ESP - 10, '7f03'), (ESP - 12, '7f0f')] * 2,
                'helper control-word save/truncation order mismatch')
        row_reads = [t for t in trace if t['kind'] == 'read' and ROWS <= int(t['address'], 16) < ROWS + ROW_COUNT * 4]
        require(len(row_reads) == 1 and row_reads[0]['address'] == hex(ROWS + rounded[0] * 4)
                and row_reads[0]['bytes'] == words(self.row_pointers[rounded[0]]).hex(), 'row pointer read mismatch')
        constant_reads = [(int(t['address'], 16), t['size']) for t in trace
                          if t['kind'] == 'read' and within(int(t['address'], 16), t['size'], CONSTANTS)]
        require(constant_reads == [(0x695070, 4), (0x695064, 4), (0x695074, 4), (0x695064, 4)],
                'constant consumption order changed')
        global_writes = [t for t in trace if t['kind'] == 'write' and int(t['address'], 16) == OUTPUT]
        require(len(global_writes) == 1 and global_writes[0]['bytes'] == words(pointer).hex(), 'output write order/value mismatch')
        after_stack = bytes(uc.mem_read(STACK, 0x10000))
        require(all(a == b or within(STACK + i, 1, stack_writes) for i, (a, b) in enumerate(zip(before_stack, after_stack))),
                'stack outside declared helper/call scratch changed')
        for start, end in CODE + CONSTANTS + [(TABLE, ROWS + ROW_COUNT * 4)]:
            require(bytes(uc.mem_read(start, end - start)) == image.read_disk(start, end - start), 'code/constants/tables changed')
        window = image.read_disk(captured_pointer, WINDOW_SIZE)
        require(1 <= offset <= 89 and offset + WINDOW_SIZE <= ROW_SIZE, 'selected window escaped source row')
        return {'name': name, 'angle_bits_inclination_azimuth': [hex(v) for v in angle_bits],
                'seed_raw80_st0_st1': [[hex(m), hex(e)] for m, e in seeds], 'seed_tags': hex(tags),
                'original_qword_results': helper_values, 'row': rounded[0], 'azimuth_before_wrap': rounded[1],
                'selected_pointer': hex(captured_pointer), 'row_window_offset': offset, 'window_sha256': sha(window),
                'window_min_max': [min(window), max(window)], 'instructions': count,
                'x87_control_word_before_after': hex(control_word), 'x87_status_after': hex(status),
                'allowed_code': [[hex(a), hex(b)] for a, b in CODE],
                'allowed_reads': [[hex(a), hex(b)] for a, b in reads], 'allowed_writes': [[hex(a), hex(b)] for a, b in writes],
                'executed_instructions': [{'address': hex(a), 'count': n} for a, n in sorted(execution.items())],
                'memory_trace': trace}


def consume_pointer(probe, selected, lighting, material, deny=None, mutation=None):
    """One original draw using a captured pointer and otherwise synthetic state.

    The existing original initializer patches the original kernel. The zero
    direction step, surface, row offsets and second table are supplied inputs;
    the selector's actual output and the original first table are not replaced.
    """
    require(0 <= lighting < 2048 and 0 <= material < 256, 'bad composition record')
    pointer = int(selected['selected_pointer'], 16)
    row = selected['row']
    require(TABLE + row * ROW_SIZE < pointer <= TABLE + row * ROW_SIZE + 89, 'captured pointer outside row')
    image = probe.image
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(image.base, (image.size + 4095) & ~4095)
    uc.mem_write(image.base, image.raw[:image.header])
    for start, _, length, offset in image.sections:
        uc.mem_write(start, image.raw[offset:offset + length])
    for address, size in [(STACK, 0x10000), (STOP, 0x1000), (pixels.DATA, 0x300000)]:
        uc.mem_map(address, size)
    uc.mem_write(STACK, b'\xa5' * 0x10000)
    second = bytes((shade * 17 + value * 7 + 3) & 255 for shade in range(24) for value in range(256))
    color, depth = b'\xa5' * 12, struct.pack('<H', 65535) * 12
    supplied = []

    def supply(address, value, label):
        uc.mem_write(address, value)
        supplied.append({'address': hex(address), 'bytes': value.hex(), 'label': label})

    supply(pixels.ROWS, words(0, 4, 8), 'synthetic three-row fixed-stride surface')
    supply(pixels.TABLE, second, 'synthetic second lookup, not original RGB/palette construction')
    supply(pixels.STREAM, pixels.record(0, lighting, material), 'one synthetic direction-zero record')
    supply(0x9c8d40, words(0, 0, 0), 'supplied zero direction step; step builder is not executed')
    supply(pixels.COLOR - 16, b'\x5a' * 16 + color + b'\x5a' * 16, 'color payload plus canaries')
    supply(pixels.DEPTH - 16, b'\x5a' * 16 + depth + b'\x5a' * 16, 'depth payload plus canaries')
    for address, value in [(OUTPUT, pointer), (0x9c6b98, pixels.DEPTH), (0x9c6b9c, pixels.COLOR),
                           (0x9c6ba4, 4), (0x9c6ba8, 4), (0x9c6bb0, pixels.TABLE), (0x9c8d1c, pixels.ROWS)]:
        supply(address, words(value), 'captured selector output' if address == OUTPUT else 'explicit raster global')
    # Consume the actual selector result; only a deliberately wrong oracle moves it.
    expected_pointer = TABLE + row * ROW_SIZE if mutation == 'ignore-offset' else pointer
    shade = image.read_disk(expected_pointer + lighting, 1)[0]
    expected_color, expected_depth = bytearray(color), bytearray(depth)
    expected_color[5] = second[shade * 256 + material]
    struct.pack_into('<H', expected_depth, 10, 1)
    phase, code, reads, writes = '', [], [], []
    trace, execution, phases, total, finished = [], Counter(), [], 0, False

    def code_hook(machine, address, size, _):
        nonlocal total, finished
        if address == STOP:
            finished = True
            machine.emu_stop()
            return
        require(address in probe.instructions and within(address, size, code), f'unexpected composition code {address:#x}')
        total += 1
        require(total <= 500, 'composition instruction budget exceeded')
        execution[(phase, address)] += 1

    def memory_hook(machine, access, address, size, value, _):
        read = access == UC_MEM_READ
        require(within(address, size, reads if read else writes),
                f'unexpected composition {"read" if read else "write"} {address:#x}+{size} in {phase}')
        data = bytes(machine.mem_read(address, size)) if read else (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
        trace.append({'phase': phase, 'instruction': hex(machine.reg_read(UC_X86_REG_EIP)),
                      'kind': 'read' if read else 'write', 'address': hex(address), 'size': size, 'bytes': data.hex()})

    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_hook)

    def invoke(label, entry, args, active, allowed_reads, allowed_writes):
        nonlocal phase, code, reads, writes, finished
        phase, code, finished = label, active, False
        reads = [(ESP - 64, ESP + 28)] + allowed_reads
        writes = [(ESP - 64, ESP)] + allowed_writes
        supply(ESP, words(STOP, *args), label + ' explicit cdecl arguments')
        for register, value in SAVED.items():
            uc.reg_write(register, value)
        uc.reg_write(UC_X86_REG_ESP, ESP)
        uc.reg_write(UC_X86_REG_EFLAGS, 2)
        uc.reg_write(UC_X86_REG_FPCW, 0x037f)
        uc.emu_start(entry, STOP + 1, timeout=2000000, count=501)
        require(finished, 'composition did not reach sentinel')
        require(uc.reg_read(UC_X86_REG_ESP) == ESP + 4, 'composition cdecl cleanup changed')
        require(all(uc.reg_read(reg) == value for reg, value in SAVED.items()), 'composition callee-saved register changed')
        require(uc.reg_read(UC_X86_REG_FPCW) == 0x037f, 'composition control word changed')
        phases.append({'name': phase, 'entry': hex(entry), 'allowed_code': [[hex(a), hex(b)] for a, b in code],
                       'allowed_reads': [[hex(a), hex(b)] for a, b in reads], 'allowed_writes': [[hex(a), hex(b)] for a, b in writes]})

    patch_ranges = [(p, p + 4) for p in pixels.PATCHES]
    if deny == 'patch':
        patch_ranges.remove((pixels.PATCHES[0], pixels.PATCHES[0] + 4))
    invoke('initialize', pixels.INITIALIZE[0], [], [pixels.INITIALIZE],
           [(a, a + 4) for a in (0x9c6bb0, 0x9c8d1c, 0x9c6b98, 0x9c6b9c, 0x9c6ba4, 0x9c6ba8)], patch_ranges)
    original = image.read_disk(0x6867f0, 0x686d51 - 0x6867f0)
    patched = bytes(uc.mem_read(0x6867f0, len(original)))
    require(all(a == b or within(0x6867f0 + i, 1, patch_ranges) for i, (a, b) in enumerate(zip(original, patched))),
            'composition initializer changed non-operand code')
    code_writes = [int(t['address'], 16) for t in trace if t['phase'] == 'initialize' and t['kind'] == 'write']
    require(len(code_writes) == len(pixels.PATCHES) and set(code_writes) == set(pixels.PATCHES), 'composition operand writes changed')
    first_range, second_range = (pointer, pointer + WINDOW_SIZE), (pixels.TABLE, pixels.TABLE + len(second))
    draw_reads = [(pixels.STREAM, pixels.STREAM + 3), first_range, second_range,
                  (pixels.ROWS, pixels.ROWS + 12), (pixels.DEPTH, pixels.DEPTH + len(depth)),
                  (OUTPUT, OUTPUT + 4), (0x9c8f18, 0x9c8f1c), (0x9c8d40, 0x9c8d4c)]
    if deny == 'first-read':
        draw_reads.remove(first_range)
    if deny == 'second-read':
        draw_reads.remove(second_range)
    draw_writes = [(0x9c8f18, 0x9c8f1c), (pixels.DEPTH, pixels.DEPTH + len(depth)), (pixels.COLOR, pixels.COLOR + len(color))]
    if deny == 'color-write':
        draw_writes.remove((pixels.COLOR, pixels.COLOR + len(color)))
    invoke('draw', 0x6867f0, [256, 256, 256, 1, pixels.STREAM, 1], pixels.DRAW, draw_reads, draw_writes)
    actual_color = bytes(uc.mem_read(pixels.COLOR, len(color)))
    actual_depth = bytes(uc.mem_read(pixels.DEPTH, len(depth)))
    require(actual_color == bytes(expected_color) and actual_depth == bytes(expected_depth), 'composition pixel/depth oracle mismatch')
    lookup_reads = [t for t in trace if t['phase'] == 'draw' and t['kind'] == 'read'
                    and (within(int(t['address'], 16), t['size'], [first_range])
                         or within(int(t['address'], 16), t['size'], [second_range]))]
    require([(int(t['address'], 16), t['size'], t['bytes']) for t in lookup_reads]
            == [(pointer + lighting, 1, bytes([shade]).hex()),
                (pixels.TABLE + shade * 256 + material, 1, bytes([expected_color[5]]).hex())], 'composition ordered lookup trace mismatch')
    record_reads = [(int(t['address'], 16), t['size']) for t in trace if t['phase'] == 'draw' and t['kind'] == 'read'
                    and pixels.STREAM <= int(t['address'], 16) < pixels.STREAM + 3]
    require(record_reads == [(pixels.STREAM + i, 1) for i in range(3)], 'composition record consumption changed')
    output_writes = [(int(t['address'], 16), t['size'], t['bytes']) for t in trace if t['phase'] == 'draw' and t['kind'] == 'write'
                     and within(int(t['address'], 16), t['size'], draw_writes[1:])]
    require(output_writes == [(pixels.DEPTH + 10, 2, '0100'), (pixels.COLOR + 5, 1, bytes([expected_color[5]]).hex())],
            'composition ordered depth/color writes changed')
    for address, initial in [(pixels.COLOR, color), (pixels.DEPTH, depth)]:
        require(bytes(uc.mem_read(address - 16, 16)) == b'\x5a' * 16
                and bytes(uc.mem_read(address + len(initial), 16)) == b'\x5a' * 16, 'composition output canary changed')
    require(bytes(uc.mem_read(STACK, ESP - STACK - 64)) == b'\xa5' * (ESP - STACK - 64)
            and bytes(uc.mem_read(ESP + 28, STACK + 0x10000 - ESP - 28)) == b'\xa5' * (STACK + 0x10000 - ESP - 28),
            'composition stack canary changed')
    require(bytes(uc.mem_read(TABLE, ROWS - TABLE)) == image.read_disk(TABLE, ROWS - TABLE),
            'composition changed original first table')
    require(bytes(uc.mem_read(pixels.TABLE, len(second))) == second, 'composition changed supplied second table')
    return {'selector_case': selected['name'], 'captured_pointer': hex(pointer), 'lighting_index': lighting,
            'material_index': material, 'original_first_table_value': int(lookup_reads[0]['bytes'], 16), 'indexed_output': actual_color[5],
            'color_bytes': actual_color.hex(), 'depth_bytes': actual_depth.hex(),
            'instructions': total, 'phases': phases, 'supplied_bytes': supplied, 'memory_trace': trace,
            'executed_instructions': [{'phase': p, 'address': hex(a), 'count': n} for (p, a), n in sorted(execution.items())]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    require(capstone.__version__ == '5.0.7' and unicorn.__version__ == '2.1.4', 'requires Capstone5.0.7 and Unicorn2.1.4')
    probe = LightingProbe(args.exe)
    scales = [-exact(v) for v in CONSTANT_BITS[1:]]
    cases, rejected = [], []
    for row in range(24):
        for azimuth in range(90):
            values = [bits(float(Fraction(v) / scale)) for v, scale in zip((row, azimuth), scales)]
            require(oracle(values)[0] == [row, azimuth], 'fixture center did not select intended row/bin')
            cases.append(probe.run(f'grid-{row}-{azimuth}', values))
    for axis, count in enumerate((23, 89)):
        for index in range(count):
            threshold = bits(float(Fraction(2 * index + 1, 2) / scales[axis]))
            outcomes = set()
            for delta in (-1, 0, 1):
                values = [bits(float(Fraction(7) / scales[0])), bits(float(Fraction(31) / scales[1]))]
                values[axis] = threshold + delta
                outcomes.add(oracle(values)[0][axis])
                cases.append(probe.run(f'neighbor-{axis}-{index}-{delta}', values))
            require(outcomes == {index, index + 1}, 'neighbor fixtures do not straddle exact rational threshold')
    for axis in range(2):
        values = [0, 0]
        values[axis] = bits(2 ** -40)
        cases.append(probe.run(f'small-admitted-{axis}', values))
        # Exact powers reach the 64-significand boundary of .5 + angle*scale.
        power = scales[axis].denominator.bit_length() - 1 - 64
        values[axis] = bits(2 ** power)
        cases.append(probe.run(f'exact64-small-boundary-{axis}', values))
        for label, value in [('next-lower-word', values[axis] - 1), ('half-power', bits(2 ** (power - 1)))]:
            invalid_values = list(values)
            invalid_values[axis] = value
            try:
                probe.run(label, invalid_values)
            except RuntimeError as error:
                require('exact extended' in str(error), 'wrong small-domain boundary rejection')
                rejected.append({'name': f'{label}-{axis}', 'input_bits': [hex(v) for v in invalid_values],
                                 'rejected_before_execution': str(error)})
            else:
                raise RuntimeError('inexact small-domain boundary admitted')
        maximum = (23, 89)[axis]
        upper = bits(float(Fraction(2 * maximum + 1, 2) / scales[axis]))
        accepted, refused = 0, 0
        for delta in (-1, 0, 1):
            values = [0, 0]
            values[axis] = upper + delta
            try:
                oracle(values)
            except RuntimeError as error:
                require('outside supported' in str(error), 'wrong upper selector boundary rejection')
                rejected.append({'name': f'upper-neighbor-{axis}-{delta}', 'input_bits': [hex(v) for v in values],
                                 'rejected_before_execution': str(error)})
                refused += 1
            else:
                cases.append(probe.run(f'upper-neighbor-{axis}-{delta}', values))
                accepted += 1
        require(accepted and refused, 'upper-boundary neighbors did not cross supported domain')
    invalid = [('negative', [0x80000001, 0], 'negative'), ('negative-zero', [0, 0x80000000], 'negative'),
               ('nan', [0x7fc00001, 0], 'nonfinite'), ('infinity', [0, 0x7f800000], 'nonfinite'),
               ('subnormal', [1, 0], 'subnormal'), ('tiny-normal', [bits(2 ** -100), 0], 'exact extended'),
               ('row-overflow', [bits(2), 0], 'outside supported'), ('azimuth-overflow', [0, bits(7)], 'outside supported'),
               ('short-input', [0], 'exactly two'), ('word-overflow', [0x100000000, 0], 'invalid binary32')]
    for name, values, expected in invalid:
        try:
            probe.run(name, values)
        except RuntimeError as error:
            require(expected in str(error), 'wrong domain rejection: ' + str(error))
            rejected.append({'name': name, 'input_bits': [hex(v) for v in values], 'rejected_before_execution': str(error)})
        else:
            raise RuntimeError('domain unexpectedly admitted: ' + name)
    for control in (0x007f, 0x077f):
        try:
            probe.run('bad-control-word', [0, 0], control_word=control)
        except RuntimeError as error:
            require('control word' in str(error), 'wrong control-word rejection')
            rejected.append({'name': hex(control), 'rejected_before_execution': str(error)})
        else:
            raise RuntimeError('control word unexpectedly admitted')
    controls = []
    values = [bits(float(Fraction(31, 4) / scales[0])), bits(float(Fraction(89) / scales[1]))]
    for field, value, expected in [('deny', 'helper-code', 'control denied'), ('deny', 'row-read', 'unexpected read'),
            ('deny', 'constant-read', 'unexpected read'), ('deny', 'output-write', 'unexpected write'),
            ('mutation', 'omit-half', 'oracle mismatch'), ('mutation', 'omit-wrap', 'oracle mismatch'),
            ('mutation', 'offset-88', 'oracle mismatch'), ('mutation', 'wrong-row', 'oracle mismatch'),
            ('budget', 1, 'instruction budget exceeded')]:
        try:
            probe.run('negative-' + str(value), values, **{field: value})
        except RuntimeError as error:
            require(expected in str(error), 'wrong negative-control failure: ' + str(error))
            controls.append({'control': value, 'expected_failure': str(error)})
        else:
            raise RuntimeError('negative control unexpectedly passed: ' + str(value))
    pixel_probe = pixels.PixelProbe(args.exe)
    composition = []
    for row in range(24):
        for lighting in (0, 255, 256, 2047):
            azimuth = (row * 13) % 90
            selected = cases[row * 90 + azimuth]
            composition.append(consume_pointer(pixel_probe, selected, lighting, (row * 17 + lighting) % 256))
    for row in (0, 23):
        for azimuth in (88, 89):
            composition.append(consume_pointer(pixel_probe, cases[row * 90 + azimuth], 2047, 255))
    composition_controls = []
    selected = cases[0]
    distinguishing_index = next(i for i in range(2048) if probe.image.read_disk(TABLE + 89 + i, 1) != probe.image.read_disk(TABLE + i, 1))
    for field, value, expected in [('deny', 'patch', 'unexpected composition write'),
            ('deny', 'first-read', 'unexpected composition read'), ('deny', 'second-read', 'unexpected composition read'),
            ('deny', 'color-write', 'unexpected composition write'), ('mutation', 'ignore-offset', 'oracle mismatch')]:
        try:
            consume_pointer(pixel_probe, selected, distinguishing_index, 17, **{field: value})
        except RuntimeError as error:
            require(expected in str(error), 'wrong composition control failure: ' + str(error))
            composition_controls.append({'control': value, 'expected_failure': str(error)})
        else:
            raise RuntimeError('composition negative control unexpectedly passed: ' + str(value))
    report = {'executable_sha256': EXE_HASH, 'probe_sha256': sha(Path(__file__).read_bytes()),
              'helper_sha256': sha(Path(__file__).with_name('probe_scout_composition.py').read_bytes()),
              'pixel_helper_sha256': sha(Path(pixels.__file__).read_bytes()),
              'capstone_version': capstone.__version__, 'unicorn_version': unicorn.__version__, 'function_stubs': [],
              'scope': 'Original selector suffix and signed64 helper with two explicit binary32 angles. Separate one-record kernel consumes captured pointers using original first-table bytes and synthetic second table/step/surface. No inverse/vector/trigonometry/camera or RGB palette construction.',
              'constant_bits': [hex(v) for v in CONSTANT_BITS], 'table_sha256': TABLE_HASH,
              'table_range': [hex(TABLE), hex(ROWS)], 'row_pointers': [hex(v) for v in probe.row_pointers],
              'limits': {'instructions_per_case': 150, 'timeout_microseconds_per_case': 2000000},
              'cases': cases, 'domain_rejections': rejected, 'negative_controls': controls,
              'composition_cases': composition, 'composition_controls': composition_controls}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'cases': len(cases), 'rejections': len(rejected), 'controls': len(controls),
                      'composition_cases': len(composition), 'composition_controls': len(composition_controls),
                      'instructions': sum(c['instructions'] for c in cases), 'report_sha256': sha(args.report.read_bytes())}))


if __name__ == '__main__':
    main()
