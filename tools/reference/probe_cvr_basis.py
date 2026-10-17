"""Execute the pinned SMAC supplied-matrix to fixed-basis conversion only.

No frame composition, camera, geometry, light tables or draw code executes.
The report contains original-executable-derived bytes; keep it outside Git.
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
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from probe_scout_composition import Probe as Image, EXE_HASH, require, words, within

CODE = [(0x65a930, 0x65ab42), (0x660678, 0x66069f)]
CONSTANTS = (0x695058, 0x695068)
BASIS, SCALED, COUNTERS = 0x9c8f40, 0x9c8f7c, 0x9c8fc4
STACK, STOP, INPUT = 0x70000000, 0x71000000, 0x72000000
ESP = STACK + 0xff00
SAVED = {UC_X86_REG_EBX: 0x11111111, UC_X86_REG_ESI: 0x22222222,
         UC_X86_REG_EDI: 0x33333333, UC_X86_REG_EBP: 0x44444444}


def sha(value):
    return hashlib.sha256(value).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def oracle(raw, mutation=None):
    """Exact rational rounding oracle over the admitted finite binary32 domain."""
    require(len(raw) == 36, 'domain: exactly nine binary32 words required')
    scaled_bits, rounded = [], []
    values = struct.unpack('<9I', raw)
    for index, value in enumerate(values):
        require((value >> 23) & 255 != 255, 'domain: nonfinite binary32')
        source = struct.unpack('<f', struct.pack('<I', value))[0]
        # Multiplication by this power of two is exact for admitted values,
        # including subnormals; pack records the original binary32 store.
        try:
            stored = struct.pack('<f', source * 256)
        except OverflowError:
            raise RuntimeError('domain: scaled binary32 overflow') from None
        stored_value = struct.unpack('<f', stored)[0]
        require(abs(stored_value) <= 2147483648, 'domain: scaled value beyond signed32 support')
        exact = Fraction.from_float(stored_value)
        if mutation == 'truncate':
            result = int(exact)
        elif mutation == 'ties-even':
            result = round(exact)
        else:
            magnitude = abs(exact) + Fraction(1, 2)
            result = magnitude.numerator // magnitude.denominator
            if exact < 0:
                result = -result
        limit = 1 << (15 if index < 6 else 31)
        require(-limit <= result < limit, 'domain: rounded output outside destination width')
        scaled_bits.append(struct.unpack('<I', stored)[0])
        rounded.append(result)
    if mutation == 'transpose':
        rounded = [rounded[j * 3 + i] for i in range(3) for j in range(3)]
    return scaled_bits, rounded


class BasisProbe:
    def __init__(self, executable):
        self.image = Image(executable)
        require(self.image.read_disk(CONSTANTS[0], 16) == words(0x43800000, 0, 0xbf000000, 0x3f000000),
                'original scaling/rounding constants changed')
        dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.instructions = {}
        for start, end in CODE:
            decoded = list(dis.disasm(self.image.read_disk(start, end - start), start))
            require(decoded and decoded[-1].address + decoded[-1].size == end, 'range ends mid-instruction')
            self.instructions.update({ins.address: ins for ins in decoded})

    def run(self, name, raw, control_word=0x037f, deny=None, mutation=None, budget=2000):
        require(control_word == 0x037f, 'domain: requires nearest-even extended x87 control word')
        scaled_bits, rounded = oracle(raw, mutation)
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        image = self.image
        uc.mem_map(image.base, (image.size + 4095) & ~4095)
        uc.mem_write(image.base, image.raw[:image.header])
        for start, _, length, offset in image.sections:
            uc.mem_write(start, image.raw[offset:offset + length])
        for address, size in [(STACK, 0x10000), (STOP, 0x1000), (INPUT, 0x1000)]:
            uc.mem_map(address, size)
        uc.mem_write(STACK, b'\xa5' * 0x10000)
        uc.mem_write(INPUT, b'\x5a' * 0x1000)
        uc.mem_write(INPUT + 16, raw)
        uc.mem_write(ESP, words(STOP, INPUT + 16))
        global_start, global_size = BASIS - 16, COUNTERS + 8 - BASIS + 32
        initial_globals = b'\xa5' * global_size
        uc.mem_write(global_start, initial_globals)
        expected_globals = bytearray(initial_globals)
        for address, value in [(BASIS, struct.pack('<6h3i', *rounded)),
                               (SCALED, words(*scaled_bits)), (COUNTERS, words(0, 0))]:
            offset = address - global_start
            expected_globals[offset:offset + len(value)] = value
        stack_range = [(ESP - 20, ESP + 8)]
        reads = stack_range + [CONSTANTS, (INPUT + 16, INPUT + 52), (SCALED, SCALED + 36)]
        writes = stack_range + [(BASIS, BASIS + 24), (SCALED, SCALED + 36), (COUNTERS, COUNTERS + 8)]
        if deny == 'read':
            reads.remove((INPUT + 16, INPUT + 52))
        if deny == 'write':
            writes.remove((BASIS, BASIS + 24))
        trace, execution, count, finished = [], Counter(), 0, False

        def code_hook(machine, address, size, _):
            nonlocal count, finished
            if address == STOP:
                finished = True
                machine.emu_stop()
                return
            require(address in self.instructions and within(address, size, CODE), f'unexpected code {address:#x}')
            require(deny != 'code' or address != CODE[1][0], 'control denied conversion helper')
            count += 1
            require(count <= budget, 'instruction budget exceeded')
            execution[address] += 1

        def memory_hook(machine, access, address, size, value, _):
            read = access == UC_MEM_READ
            require(within(address, size, reads if read else writes),
                    f'unexpected {"read" if read else "write"} {address:#x}+{size}')
            data = bytes(machine.mem_read(address, size)) if read else (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
            trace.append({'kind': 'read' if read else 'write', 'address': hex(address), 'size': size, 'bytes': data.hex()})

        uc.hook_add(UC_HOOK_CODE, code_hook)
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_hook)
        for register, value in SAVED.items():
            uc.reg_write(register, value)
        uc.reg_write(UC_X86_REG_ESP, ESP)
        uc.reg_write(UC_X86_REG_EFLAGS, 2)
        uc.reg_write(UC_X86_REG_FPCW, control_word)
        uc.reg_write(UC_X86_REG_FPTAG, 0xffff)
        uc.reg_write(UC_X86_REG_FPSW, 0)
        uc.emu_start(CODE[0][0], STOP + 1, timeout=2000000)
        require(finished, 'did not reach sentinel before timeout')
        require(uc.reg_read(UC_X86_REG_ESP) == ESP + 4, 'stack cleanup changed')
        require(all(uc.reg_read(reg) == value for reg, value in SAVED.items()), 'callee-saved register changed')
        require(uc.reg_read(UC_X86_REG_FPCW) == control_word, 'x87 control word not restored')
        require(uc.reg_read(UC_X86_REG_FPSW) & 0x3800 == 0 and uc.reg_read(UC_X86_REG_FPTAG) == 0xffff,
                'x87 stack not empty after return')
        require(uc.reg_read(UC_X86_REG_FPSW) & 0x1d == 0, 'unexpected x87 invalid/divide/overflow/underflow status')
        require(execution[CODE[1][0]] == 9, 'helper call count changed')
        actual_globals = bytes(uc.mem_read(global_start, global_size))
        require(actual_globals == bytes(expected_globals), 'basis/scaled/counter byte oracle mismatch')
        expected_writes = [(SCALED + i * 4, words(v)) for i, v in enumerate(scaled_bits)]
        expected_writes += [(BASIS + i * 2, struct.pack('<h', v)) for i, v in enumerate(rounded[:6])]
        expected_writes += [(BASIS + 12 + i * 4, struct.pack('<i', v)) for i, v in enumerate(rounded[6:])]
        expected_writes += [(COUNTERS, words(0)), (COUNTERS + 4, words(0))]
        global_writes = [(int(t['address'], 16), bytes.fromhex(t['bytes'])) for t in trace
                         if t['kind'] == 'write' and global_start <= int(t['address'], 16) < global_start + global_size]
        require(global_writes == expected_writes, 'global write order/value mismatch')
        helper_values = [struct.unpack('<q', bytes.fromhex(t['bytes']))[0] for t in trace
                         if t['kind'] == 'write' and int(t['address'], 16) == ESP - 20 and t['size'] == 8]
        require(helper_values == rounded, 'original conversion qword mismatch')
        control_writes = [(int(t['address'], 16), t['bytes']) for t in trace
                          if t['kind'] == 'write' and int(t['address'], 16) in (ESP - 10, ESP - 12)]
        require(control_writes == [(ESP - 10, '7f03'), (ESP - 12, '7f0f')] * 9,
                'original helper control-word save/truncation order mismatch')
        input_reads = [(int(t['address'], 16), t['size']) for t in trace
                       if t['kind'] == 'read' and INPUT <= int(t['address'], 16) < INPUT + 0x1000]
        require(input_reads == [(INPUT + 16 + i * 4, 4) for i in range(9)], 'input word order/count changed')
        require(bytes(uc.mem_read(INPUT, 0x1000)) == b'\x5a' * 16 + raw + b'\x5a' * (0x1000 - 52), 'input/canary changed')
        require(bytes(uc.mem_read(STACK, 0xfeec)) == b'\xa5' * 0xfeec, 'lower stack canary changed')
        require(bytes(uc.mem_read(ESP + 8, 0xf8)) == b'\xa5' * 0xf8, 'upper stack canary changed')
        for start, end in CODE + [CONSTANTS]:
            require(bytes(uc.mem_read(start, end - start)) == image.read_disk(start, end - start), 'code/constants changed')
        return {'name': name, 'input_bits': [hex(v) for v in struct.unpack('<9I', raw)],
                'scaled_bits': [hex(v) for v in scaled_bits], 'basis': rounded, 'original_qword_results': helper_values,
                'input_bytes': raw.hex(), 'global_bytes': actual_globals.hex(), 'instructions': count,
                'x87_control_word_before_after': hex(control_word), 'x87_status_after': hex(uc.reg_read(UC_X86_REG_FPSW)),
                'allowed_code': [[hex(a), hex(b)] for a, b in CODE],
                'allowed_reads': [[hex(a), hex(b)] for a, b in reads],
                'allowed_writes': [[hex(a), hex(b)] for a, b in writes],
                'executed_instructions': [{'address': hex(a), 'count': n} for a, n in sorted(execution.items())],
                'memory_trace': trace}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    require(capstone.__version__ == '5.0.7' and unicorn.__version__ == '2.1.4', 'requires Capstone5.0.7 and Unicorn2.1.4')
    probe = BasisProbe(args.exe)
    cases = []
    def add(name, values):
        cases.append(probe.run(name, words(*values)))
    add('identity', [bits(v) for v in (1, 0, 0, 0, 1, 0, 0, 0, 1)])
    add('signed-zero', [0, 0x80000000] * 4 + [0])
    add('nonidentity-distinct', [bits(v) for v in (1.25, -2.5, 3.125, -.25, .75, -4.5, 4000.25, -8000.5, 17.125)])
    # Neighboring binary32 values on both sides of exact half-integer thresholds.
    for center in (.5 / 256, 1.5 / 256, 2.5 / 256, 32766.5 / 256):
        raw = bits(center)
        add('threshold-' + hex(raw), [raw - 1, raw, raw + 1, (raw - 1) | 0x80000000,
                                     raw | 0x80000000, (raw + 1) | 0x80000000, raw, raw | 0x80000000, raw + 1])
    add('finite-small', [1, 0x80000001, 0x007fffff, 0x807fffff, 0x00800000, 0x80800000, 0x33800000, 0xb3800000, 0x00010000])
    add('destination-boundaries', [bits(127.998046875) - 1, bits(-128), bits(127.99609375),
                                  bits(-127.998046875), bits(0), bits(-0.0), bits(8388608) - 1,
                                  bits(-8388608), bits(8388607)])
    # Deterministic mixed matrices: every position differs; no asset matrix is
    # implicitly interpreted as the caller's already-composed input.
    for seed in range(32):
        add('mixed-' + str(seed), [bits((((seed + 3) * (cell + 7) * 37) % 4001 - 2000) / 32)
                                  for cell in range(9)])
    rejected = []
    for name, value, index, expected in [('nan', 0x7fc00001, 0, 'nonfinite'),
            ('positive-infinity', 0x7f800000, 8, 'nonfinite'), ('negative-infinity', 0xff800000, 0, 'nonfinite'),
            ('scaled-overflow', 0x7f7fffff, 8, 'overflow'),
            ('signed16-upper-half', bits(127.998046875), 0, 'destination width'),
            ('signed16-lower-half', bits(-128.001953125), 5, 'destination width'),
            ('signed32-upper', bits(8388608), 6, 'destination width'),
            ('signed32-lower', bits(-8388608) + 1, 8, 'signed32 support')]:
        values = [0] * 9
        values[index] = value
        try:
            probe.run('domain-' + name, words(*values))
        except RuntimeError as error:
            require(expected in str(error), 'wrong domain rejection: ' + str(error))
            rejected.append({'name': name, 'input_bits': [hex(v) for v in values], 'rejected_before_execution': str(error)})
        else:
            raise RuntimeError('domain unexpectedly admitted: ' + name)
    for name, raw, control_word, expected in [('short-input', bytes(32), 0x037f, 'nine binary32'),
            ('different-precision', bytes(36), 0x007f, 'control word'), ('different-rounding', bytes(36), 0x077f, 'control word')]:
        try:
            probe.run(name, raw, control_word=control_word)
        except RuntimeError as error:
            require(expected in str(error), 'wrong domain rejection: ' + str(error))
            rejected.append({'name': name, 'rejected_before_execution': str(error)})
        else:
            raise RuntimeError('domain unexpectedly admitted: ' + name)
    controls = []
    raw = words(*[bits(v / 256) for v in (2.5, -2.5, 1.5, -1.5, 3.5, -3.5, 12.5, -13.5, 14.5)])
    for field, value, expected in [('deny', 'code', 'control denied'), ('deny', 'read', 'unexpected read'),
            ('deny', 'write', 'unexpected write'), ('mutation', 'truncate', 'byte oracle mismatch'),
            ('mutation', 'ties-even', 'byte oracle mismatch'), ('mutation', 'transpose', 'byte oracle mismatch'),
            ('budget', 1, 'instruction budget exceeded')]:
        try:
            probe.run('negative-' + str(value), raw, **{field: value})
        except RuntimeError as error:
            require(expected in str(error), 'wrong negative control failure: ' + str(error))
            controls.append({'control': value, 'expected_failure': str(error)})
        else:
            raise RuntimeError('negative control unexpectedly passed: ' + str(value))
    report = {'executable_sha256': EXE_HASH, 'probe_sha256': sha(Path(__file__).read_bytes()),
              'helper_sha256': sha(Path(__file__).with_name('probe_scout_composition.py').read_bytes()),
              'capstone_version': capstone.__version__, 'unicorn_version': unicorn.__version__, 'function_stubs': [],
              'scope': 'Original supplied-matrix to fixed-basis conversion and original truncating x87 helper; explicit nine binary32 inputs and FPCW037f. No caller/camera/geometry/render execution.',
              'limits': {'instructions_per_case': 2000, 'timeout_microseconds_per_case': 2000000},
              'code': [{'start': hex(a), 'end': hex(b), 'sha256': sha(probe.image.read_disk(a, b - a))} for a, b in CODE],
              'cases': cases, 'domain_rejections': rejected, 'negative_controls': controls}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'cases': len(cases), 'domain_rejections': len(rejected), 'negative_controls': len(controls),
                      'instructions': sum(c['instructions'] for c in cases), 'report_sha256': sha(args.report.read_bytes())}))


if __name__ == '__main__':
    main()
