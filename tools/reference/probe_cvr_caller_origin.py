"""Execute two bounded SMAC caller/origin fragments with supplied transform state.

Fragment A stops BEFORE lighting. Fragment B starts separately with A's captured
binary32 arguments. This is not continuous execution of the original renderer.
Reports contain original-derived data and must remain outside Git.
"""
from fractions import Fraction
from pathlib import Path
import argparse
import hashlib
import itertools
import json
import struct

import capstone
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from probe_scout_composition import Probe as Image, EXE_HASH, ASSETS, require, words, within, scene
from probe_cvr_pixels import installed_stream
from probe_cvr_basis import oracle as basis_oracle

COMMON = [(0x641e70, 0x641edb), (0x641ee0, 0x641f05), (0x660678, 0x66069f)]
A_CODE = COMMON + [(0x654849, 0x6549ee), (0x641bb0, 0x641cdf),
    (0x652a50, 0x652a91), (0x6420a0, 0x642137), (0x659800, 0x659827), (0x65a930, 0x65ab42)]
B_CODE = COMMON + [(0x659c75, 0x659d65)]
CONSTANTS = [(0x694c38, 0x694c3c), (0x695030, 0x695034), (0x695058, 0x695068)]
BASIS, SCALED, COUNTERS = 0x9c8f40, 0x9c8f7c, 0x9c8fc4
TRANSLATION, TRANSFORM, SCALE = 0x9c8fcc, 0x9c8fd8, 0x9c9000
STACK, DATA, ESP = 0x70000000, 0x72000000, 0x7000ff00
CONTEXT, MODEL, CAMERA, TRIPLE, DESCRIPTOR = [DATA + n for n in (0x100, 0x200, 0x300, 0x400, 0x420)]
FLAGS, LOCATIONS, MATRICES, GEOMETRY = [DATA + n for n in (0x500, 0x600, 0x1000, 0x2000)]
CAPTURE_LOCATION, CAPTURE_MATRIX = DATA + 0x3000, DATA + 0x3040
REGS = (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP)
COEFFICIENTS = [v for v in itertools.product((-1, 0, 1), repeat=3) if v != (0, 0, 0)]


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def fbytes(values):
    return struct.pack('<' + 'f' * len(values), *values)


def rationals(raw):
    require(len(raw) % 4 == 0, 'domain: partial binary32 input')
    result = []
    for (bits,) in struct.iter_unpack('<I', raw):
        require((bits >> 23) & 255 != 255, 'domain: nonfinite binary32')
        require(bits != 0x80000000, 'domain: negative zero outside this arithmetic oracle')
        result.append(Fraction.from_float(struct.unpack('<f', words(bits))[0]))
    return result


def exact_extended(value):
    """Admit only exact extended intermediates; do not guess x87 rounding."""
    magnitude = abs(value.numerator)
    if magnitude:
        magnitude >>= (magnitude & -magnitude).bit_length() - 1
    require(magnitude.bit_length() <= 64, 'domain: inexact extended intermediate')
    require(value.denominator & (value.denominator - 1) == 0, 'domain: nondyadic intermediate')
    return value


def nearest_even(numerator, denominator):
    quotient, remainder = divmod(numerator, denominator)
    return quotient + (2 * remainder > denominator or (2 * remainder == denominator and quotient & 1))


def store(value):
    """One exact-rational to normal binary32 nearest-even store, no double round."""
    exact_extended(value)
    if value == 0:
        return words(0)
    sign = 0x80000000 if value < 0 else 0
    value = abs(value)
    exponent = value.numerator.bit_length() - value.denominator.bit_length()
    power = Fraction(2) ** exponent
    if value < power:
        exponent -= 1
    require(-126 <= exponent <= 127, 'domain: nonnormal binary32 store')
    scaled = value / (Fraction(2) ** (exponent - 23))
    significand = nearest_even(scaled.numerator, scaled.denominator)
    if significand == 1 << 24:
        significand >>= 1
        exponent += 1
    require(exponent <= 127, 'domain: binary32 store overflow')
    return words(sign | ((exponent + 127) << 23) | (significand - (1 << 23)))


def dot(left, right):
    products = [exact_extended(a * b) for a, b in zip(left, right)]
    # Check every pair as the original helper schedules different cells in
    # different orders. With every sum exact, associativity is safe here only.
    for a, b in itertools.combinations(products, 2):
        exact_extended(a + b)
    return store(exact_extended(sum(products)))


def matvec(matrix, vector):
    m, v = rationals(matrix), rationals(vector)
    require(len(m) == 9 and len(v) == 3, 'domain: matrix/vector shape')
    return b''.join(dot(m[i:i + 3], v) for i in (0, 3, 6))


def matmul(left, right):
    a, b = rationals(left), rationals(right)
    require(len(a) == len(b) == 9, 'domain: matrix shape')
    return b''.join(dot(a[i:i + 3], b[j::3]) for i in (0, 3, 6) for j in range(3))


def add(left, right):
    return b''.join(store(a + b) for a, b in zip(rationals(left), rationals(right)))


def round_origin(raw):
    result = []
    for value in rationals(raw):
        scaled = exact_extended(value * 256)
        shifted = exact_extended(abs(scaled) + Fraction(1, 2))
        integer = shifted.numerator // shifted.denominator
        integer = -integer if value < 0 else integer
        require(-(1 << 31) <= integer < 1 << 31, 'domain: origin outside signed32')
        result.append(integer)
    return result


def arithmetic(frame_location, frame_matrix, transform, translation, header, mutation=None):
    require(len(header) == 42 and struct.unpack_from('<I', header)[0] == 8, 'domain: only flags-8 header')
    scalar = rationals(header[8:12])[0]
    require(scalar > 0, 'domain: positive scalar required')
    transformed = matvec(transform, frame_location)
    location = add(transformed, translation if mutation != 'omit-translation' else bytes(12))
    product = matmul(transform, frame_matrix) if mutation != 'reverse-product' else matmul(frame_matrix, transform)
    matrix = b''.join(store(v * (1 if mutation == 'omit-scalar' else scalar)) for v in rationals(product))
    if mutation == 'transpose':
        matrix = b''.join(matrix[(j * 3 + i) * 4:(j * 3 + i + 1) * 4] for i in range(3) for j in range(3))
    vector = fbytes(struct.unpack_from('<3h', header, 32) if mutation != 'omit-header-vector' else (0, 0, 0))
    header_transform = matvec(matrix, vector)
    origin_float = add(location, header_transform)
    scaled, basis = basis_oracle(matrix)
    return {'transformed': transformed, 'location': location, 'product': product, 'matrix': matrix,
            'vector': vector, 'header_transform': header_transform, 'origin_float': origin_float,
            'basis': basis, 'scaled': scaled, 'origin': round_origin(origin_float)}


def raster_domain(basis, origin, stream):
    """Separate GLSMAC checked-arithmetic policy, no executed draw or viewport."""
    fits = lambda v: -(1 << 31) <= v < 1 << 31
    steps = [[sum(basis[row * 3 + col] * coefficient[col] for col in range(3))
              for row in range(3)] for coefficient in COEFFICIENTS]
    if not all(fits(v) for step in steps for v in step) or not all(fits(step[2] * 256) for step in steps):
        return {'admissible': False, 'reason': 'step or shifted depth outside signed32'}
    position = [origin[0], origin[1], origin[2] * 256]
    if not all(fits(v) for v in position):
        return {'admissible': False, 'reason': 'initial shifted depth outside signed32'}
    minimum, maximum = position[:], position[:]
    for index in range(0, len(stream) - 3, 3):
        code = stream[index] >> 3
        require(code < 26, 'invalid supplied raster classification record')
        step = steps[code]
        position = [position[0] + step[0], position[1] + step[1], position[2] + step[2] * 256]
        if not all(fits(v) for v in position):
            return {'admissible': False, 'reason': 'cumulative coordinate outside signed32', 'record': index // 3}
        minimum = [min(a, b) for a, b in zip(minimum, position)]
        maximum = [max(a, b) for a, b in zip(maximum, position)]
    return {'admissible': True, 'records_checked': len(stream) // 3 - 1, 'minimum': minimum,
            'maximum': maximum, 'surface_bounds_checked': False}


class CallerProbe:
    def __init__(self, executable):
        self.image = Image(executable)
        require(self.image.read_disk(CONSTANTS[0][0], 4) == fbytes([128]), 'margin constant changed')
        require(self.image.read_disk(CONSTANTS[1][0], 4) == fbytes([.5]), 'footprint threshold changed')
        require(self.image.read_disk(CONSTANTS[2][0], 16) == fbytes([256, 0, -.5, .5]), 'rounding constants changed')
        self.instructions = {}
        decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        for start, end in A_CODE + B_CODE:
            decoded = list(decoder.disasm(self.image.read_disk(start, end - start), start))
            require(decoded and decoded[-1].address + decoded[-1].size == end, 'code range ends mid-instruction')
            self.instructions.update({ins.address: ins for ins in decoded})

    def phase(self, phase, supplied, registers, reads, writes, checkpoints, deny=None, budget=4000):
        image = self.image
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(image.base, (image.size + 4095) & ~4095)
        uc.mem_write(image.base, image.raw[:image.header])
        for start, _, length, offset in image.sections:
            uc.mem_write(start, image.raw[offset:offset + length])
        for address, size in ((STACK, 0x10000), (DATA, 0x4000)):
            uc.mem_map(address, size)
            uc.mem_write(address, b'\xa5' * size)
        uc.mem_write(BASIS - 16, b'\xa5' * (SCALE + 20 - BASIS))
        for address, raw, _ in supplied:
            uc.mem_write(address, raw)
        stack_extent = (ESP - (0xb0 if phase == 'A' else 0x20), ESP + 0x80)
        reads = [stack_extent] + reads
        writes = [stack_extent] + writes
        allowed_code = A_CODE if phase == 'A' else B_CODE
        entry, stop = (0x654849, 0x659827) if phase == 'A' else (0x659c75, 0x659d65)
        snapshots = [(STACK, bytes(uc.mem_read(STACK, 0x10000))),
                     (DATA, bytes(uc.mem_read(DATA, 0x4000))),
                     (BASIS - 16, bytes(uc.mem_read(BASIS - 16, SCALE + 20 - BASIS)))]
        trace, execution, captured, qwords, helper_calls, active_calls = [], [], {}, [], [], []
        helper_control_writes = []
        stopped = False
        helper_stack = None
        if deny == 'read':
            reads.remove((TRANSFORM, TRANSFORM + 36)) if phase == 'A' else reads.remove((CAPTURE_MATRIX, CAPTURE_MATRIX + 36))
        if deny == 'write':
            writes = [r for r in writes if r != (BASIS, BASIS + 24)] if phase == 'A' else [(ESP - 0x20, ESP + 0x18)]

        def code_hook(machine, address, size, _):
            nonlocal stopped, helper_stack
            if active_calls and address == active_calls[-1]['return']:
                call = active_calls.pop()
                require(machine.reg_read(UC_X86_REG_ESP) == call['esp'] + 4, 'helper stack cleanup changed')
                require(all(machine.reg_read(reg) == value for reg, value in call['saved'].items()),
                        'helper callee-saved register changed')
            if address == stop:
                stopped = True
                machine.emu_stop()
                return
            require(address in self.instructions and within(address, size, allowed_code), f'unexpected code {address:#x}')
            require(deny != 'code' or address != 0x641e70, 'control denied matrix helper')
            execution.append(address)
            require(len(execution) <= budget, 'instruction budget exceeded')
            if address in checkpoints:
                for name, location, size_bytes, expected in checkpoints[address]:
                    raw = bytes(machine.mem_read(location, size_bytes))
                    require(raw == expected, 'stage oracle mismatch: ' + name)
                    captured[name] = raw.hex()
            if address in (0x641bb0, 0x641e70, 0x641ee0, 0x652a50, 0x6420a0, 0x659800, 0x65a930, 0x660678):
                helper_calls.append(hex(address))
                if address != 0x659800:  # intentionally incomplete renderer invocation
                    current = machine.reg_read(UC_X86_REG_ESP)
                    active_calls.append({'esp': current, 'return': struct.unpack('<I', machine.mem_read(current, 4))[0],
                        'saved': {reg: machine.reg_read(reg) for reg in (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)}})
            if address == 0x659800:
                current = machine.reg_read(UC_X86_REG_ESP)
                captured['renderer_arguments'] = bytes(machine.mem_read(current + 4, 32)).hex()
            if address == 0x660678:
                helper_stack = machine.reg_read(UC_X86_REG_ESP)

        def memory_hook(machine, access, address, size, value, _):
            read = access == UC_MEM_READ
            require(within(address, size, reads if read else writes), f'unexpected {"read" if read else "write"} {address:#x}+{size}')
            raw = bytes(machine.mem_read(address, size)) if read else (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
            trace.append({'pc': hex(machine.reg_read(UC_X86_REG_EIP)), 'kind': 'read' if read else 'write',
                          'address': hex(address), 'size': size, 'bytes': raw.hex()})
            if not read and size == 8 and machine.reg_read(UC_X86_REG_EIP) == 0x660691:
                require(address == helper_stack - 16, 'helper qword destination changed')
                qwords.append(struct.unpack('<q', raw)[0])
            if not read and size == 2 and machine.reg_read(UC_X86_REG_EIP) in (0x66067f, 0x66068a):
                helper_control_writes.append(raw.hex())

        uc.hook_add(UC_HOOK_CODE, code_hook)
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_hook)
        for register in REGS:
            uc.reg_write(register, registers.get(register, 0x11111111))
        uc.reg_write(UC_X86_REG_ESP, ESP)
        uc.reg_write(UC_X86_REG_EFLAGS, 2)
        uc.reg_write(UC_X86_REG_FPCW, 0x037f)
        uc.reg_write(UC_X86_REG_FPTAG, 0xffff)
        uc.reg_write(UC_X86_REG_FPSW, 0)
        uc.emu_start(entry, stop + 1, timeout=2000000)
        require(stopped, 'fragment did not reach boundary')
        require(not active_calls, 'helper did not return before boundary')
        require(helper_control_writes == ['7f03', '7f0f'] * (9 if phase == 'A' else 3), 'helper control save/truncation changed')
        require(uc.reg_read(UC_X86_REG_ESP) == ESP - (0x94 if phase == 'A' else 0), 'fragment stack boundary changed')
        require(uc.reg_read(UC_X86_REG_FPCW) == 0x037f, 'x87 control changed')
        status = uc.reg_read(UC_X86_REG_FPSW)
        require(status & 0x3800 == 0 and uc.reg_read(UC_X86_REG_FPTAG) == 0xffff, 'x87 stack not empty')
        require(status & 0x1d == 0, 'unexpected x87 invalid/divide/overflow/underflow')
        for start, before in snapshots:
            expected = bytearray(before)
            for access in trace:
                address = int(access['address'], 16)
                if access['kind'] == 'write' and start <= address < start + len(before):
                    expected[address - start:address - start + access['size']] = bytes.fromhex(access['bytes'])
            require(bytes(uc.mem_read(start, len(before))) == expected, 'unobserved memory or canary change')
        for start, end in allowed_code + CONSTANTS:
            require(bytes(uc.mem_read(start, end - start)) == image.read_disk(start, end - start), 'code/constants changed')
        capture = {'matrix': bytes(uc.mem_read(ESP + 0x30, 36)), 'location': bytes(uc.mem_read(ESP + 0x24, 12))} if phase == 'A' else {}
        capture['basis'] = list(struct.unpack('<6h3i', uc.mem_read(BASIS, 24))) if phase == 'A' else []
        if phase == 'A':
            capture['scaled'] = list(struct.unpack('<9I', uc.mem_read(SCALED, 36)))
            require(bytes(uc.mem_read(COUNTERS, 8)) == bytes(8), 'basis scratch counters changed')
        if phase == 'B':
            capture['origin'] = [struct.unpack('<i', uc.mem_read(ESP + 0x7c, 4))[0]] + [
                struct.unpack('<i', words(uc.reg_read(reg)))[0] for reg in (UC_X86_REG_EBP, UC_X86_REG_EAX)]
            capture['origin_float'] = bytes(uc.mem_read(ESP + 0x18, 12))
        return capture, {'entry': hex(entry), 'halt_before': hex(stop), 'instructions': len(execution),
            'executed_instructions': [hex(a) for a in execution], 'memory_trace': trace,
            'supplied': [{'address': hex(a), 'bytes': b.hex(), 'reason': reason} for a, b, reason in supplied],
            'allowed_code': [[hex(a), hex(b)] for a, b in allowed_code],
            'allowed_reads': [[hex(a), hex(b)] for a, b in reads], 'allowed_writes': [[hex(a), hex(b)] for a, b in writes],
            'checkpoint_bytes': captured, 'helper_calls': helper_calls, 'original_qword_results': qwords,
            'helper_control_writes': helper_control_writes,
            'final_registers': [hex(uc.reg_read(reg)) for reg in REGS], 'x87_status_after': hex(status),
            'x87_control_before_after': '0x037f', 'x87_empty_at_boundary': True}

    def run(self, name, obj, frame, transform, translation, header, stream, scale=1,
            mutation=None, deny=None, deny_phase='A', budget=4000):
        require(0 <= frame < len(obj['flags']), 'domain: frame outside supplied array')
        require(obj['flags'][frame] & 0x80 == 0, 'domain: hidden frame')
        require(Fraction(scale) > 0, 'domain: positive admission scale')
        require(len(obj['locations']) == 12 * len(obj['flags']) and len(obj['matrices']) == 36 * len(obj['flags']),
                'domain: frame array extent')
        require(len(obj['flags']) <= 32, 'domain: frame array exceeds bounded arena')
        require(len(transform) == 36 and len(translation) == 12, 'domain: transform/translation shape')
        frame_location = obj['locations'][frame * 12:frame * 12 + 12]
        frame_matrix = obj['matrices'][frame * 36:frame * 36 + 36]
        expected = arithmetic(frame_location, frame_matrix, transform, translation, header, mutation)
        supplied = [(CONTEXT + 0x58, words(0), 'explicitly disable optional pre-draw callback'),
            (MODEL + 4, words(1), 'one supplied selected object'), (MODEL + 12, words(TRIPLE, DESCRIPTOR), 'loaded ABI arrays'),
            (TRIPLE, words(FLAGS, LOCATIONS, MATRICES), 'frame arrays'), (DESCRIPTOR, words(0x12345678, GEOMETRY), 'opaque handle and simple geometry pointer'),
            (FLAGS, obj['flags'], 'supplied frame flags'), (LOCATIONS, obj['locations'], 'supplied frame locations'),
            (MATRICES, obj['matrices'], 'supplied frame matrices'), (GEOMETRY, header, 'simple flags-8 header'),
            (TRANSFORM, transform, 'supplied transform, not a default camera'), (TRANSLATION, translation, 'supplied translation'),
            (SCALE, fbytes([scale]), 'supplied admission/footprint scale'),
            (CAMERA + 0x40, fbytes([-100000, 100000]), 'supplied permissive depth bounds'),
            (CAMERA + 0x58, words(100000, 100000), 'supplied positive x/y extents'),
            (ESP + 0x5c, words(MODEL, CAMERA, 0), 'explicit loop ABI: model, camera, object index')]
        reads = CONSTANTS + [(a, a + len(b)) for a, b, _ in supplied
            if not STACK <= a < STACK + 0x10000 and a not in (FLAGS, LOCATIONS, MATRICES, GEOMETRY)]
        reads += [(FLAGS + frame, FLAGS + frame + 1), (LOCATIONS + frame * 12, LOCATIONS + (frame + 1) * 12),
                  (MATRICES + frame * 36, MATRICES + (frame + 1) * 36), (GEOMETRY, GEOMETRY + 4), (GEOMETRY + 8, GEOMETRY + 12)]
        reads += [(SCALED, SCALED + 36)]
        writes = [(BASIS, BASIS + 24), (SCALED, SCALED + 36), (COUNTERS, COUNTERS + 8)]
        checks = {0x654887: [('transformed_location', ESP + 0x24, 12, expected['transformed'])],
                  0x65489e: [('translated_location', ESP + 0x24, 12, expected['location'])],
                  0x65491e: [('matrix_product', ESP + 0x30, 36, expected['product'])],
                  0x659800: [('scaled_matrix', ESP + 0x30, 36, expected['matrix'])]}
        a, report_a = self.phase('A', supplied, {UC_X86_REG_EDI: CONTEXT, UC_X86_REG_EBX: MODEL,
            UC_X86_REG_EBP: frame, UC_X86_REG_ECX: 0}, reads, writes, checks,
            deny if deny_phase == 'A' else None, budget if deny_phase == 'A' else 4000)
        require(a['matrix'] == expected['matrix'] and a['location'] == expected['location'], 'A capture mismatch')
        require(a['basis'] == expected['basis'] == report_a['original_qword_results'], 'A basis qword mismatch')
        require(a['scaled'] == expected['scaled'], 'A scaled binary32 store mismatch')
        expected_writes = [(SCALED + i * 4, words(v).hex()) for i, v in enumerate(expected['scaled'])]
        expected_writes += [(BASIS + i * 2, struct.pack('<h', v).hex()) for i, v in enumerate(expected['basis'][:6])]
        expected_writes += [(BASIS + 12 + i * 4, struct.pack('<i', v).hex()) for i, v in enumerate(expected['basis'][6:])]
        expected_writes += [(COUNTERS, words(0).hex()), (COUNTERS + 4, words(0).hex())]
        actual_writes = [(int(access['address'], 16), access['bytes']) for access in report_a['memory_trace']
            if access['kind'] == 'write' and BASIS <= int(access['address'], 16) < COUNTERS + 8]
        require(actual_writes == expected_writes, 'basis global write order changed')
        footprint = int(rationals(header[8:12])[0] * Fraction(scale) <= Fraction(1, 2))
        require(report_a['checkpoint_bytes']['renderer_arguments'] == words(CONTEXT, GEOMETRY, ESP + 0x24,
            ESP + 0x30, CAMERA + 12, MODEL + 0x48, struct.unpack_from('<I', header, 8)[0], footprint).hex(), 'actual renderer argument mismatch')
        expected_calls = ['0x641e70', '0x641ee0', '0x652a50', '0x6420a0', '0x641bb0', '0x659800', '0x65a930'] + ['0x660678'] * 9
        require(report_a['helper_calls'] == expected_calls, 'A call order changed')
        # Fresh emulator: A's captured bytes, not the oracle's recomputed bytes.
        supplied_b = [(GEOMETRY, header, 'same geometry header'), (CAPTURE_MATRIX, a['matrix'], 'captured A matrix bytes'),
            (CAPTURE_LOCATION, a['location'], 'captured A location bytes'),
            (ESP + 0x6c, words(CAPTURE_LOCATION), 'separately seeded origin-fragment ABI')]
        b, report_b = self.phase('B', supplied_b, {UC_X86_REG_ECX: GEOMETRY, UC_X86_REG_EBP: CAPTURE_MATRIX},
            [(0x695058, 0x695068), (GEOMETRY + 32, GEOMETRY + 38)] + [(a, a + len(raw)) for a, raw, _ in supplied_b
                if not STACK <= a < STACK + 0x10000 and a != GEOMETRY], [],
            {0x659cb5: [('header_vector', ESP + 0x18, 12, expected['vector']),
                       ('transformed_header_vector', ESP + 0x24, 12, expected['header_transform'])],
             0x659ccc: [('origin_float', ESP + 0x18, 12, expected['origin_float'])]},
            deny if deny_phase == 'B' else None, budget)
        require(b['origin_float'] == expected['origin_float'], 'B origin float mismatch')
        require(b['origin'] == expected['origin'] == report_b['original_qword_results'], 'B origin qword mismatch')
        require(report_b['helper_calls'] == ['0x641e70', '0x641ee0'] + ['0x660678'] * 3, 'B call order changed')
        return {'name': name, 'frame': frame, 'frame_location_bytes': frame_location.hex(), 'frame_matrix_bytes': frame_matrix.hex(),
            'header_bytes': header.hex(), 'supplied_transform_bytes': transform.hex(), 'supplied_translation_bytes': translation.hex(),
            'admission_scale': scale, 'footprint_argument': footprint, 'captured_matrix_bytes': a['matrix'].hex(),
            'captured_location_bytes': a['location'].hex(), 'basis': a['basis'], 'origin_float_bytes': b['origin_float'].hex(),
            'origin': b['origin'], 'raster_arithmetic_policy': raster_domain(a['basis'], b['origin'], stream), 'A': report_a, 'B': report_b}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    require(capstone.__version__ == '5.0.7' and unicorn.__version__ == '2.1.4', 'requires Capstone5.0.7/Unicorn2.1.4')
    probe = CallerProbe(args.exe)
    asset = scene(args.assets / 'VI.cvr', ASSETS['vi.cvr'][0])
    obj = asset['objects'][0]
    require(obj['name'] == 'EH_M1-Mask05' and asset['frames'] == 17, 'selected scene changed')
    stream, metadata = installed_stream(args.assets, 'vi.cvr', obj['name'])
    header = bytes.fromhex(metadata['raw_header'])
    transform = fbytes([.5, .125, -.25, -.375, .75, .125, .125, -.25, .5])
    translation = fbytes([2048, 2048, 1024])
    cases = []
    for frame in (0, 1, 8, 14, 16):
        cases.append(probe.run('installed-frame-' + str(frame), obj, frame, transform, translation, header, stream))
    synthetic = {'flags': bytes(3), 'locations': fbytes([3, 5, -7, -4, 11, 2, 9, -6, 3]),
        'matrices': fbytes([1, .25, -.5, -.125, 1.5, .75, .625, -.25, 2,
                           -.25, 1, .5, 1.25, -.75, .125, .375, 2, -1,
                           .5, -.25, 1.5, 2, .125, -.75, -.625, 1, .25])}
    for frame in range(3):
        cases.append(probe.run('noncommuting-frame-' + str(frame), synthetic, frame, transform, translation, header, stream))
    for scalar in (.25, 1, 2, 3):
        modified = bytearray(header)
        modified[8:12] = fbytes([scalar])
        cases.append(probe.run('scalar-' + str(scalar), synthetic, 1, transform, translation, bytes(modified), stream))
    for vector in ((0, 0, 0), (7, -11, 13), (-32768, 32767, -1)):
        modified = bytearray(header)
        struct.pack_into('<3h', modified, 32, *vector)
        cases.append(probe.run('header-vector-' + str(vector), synthetic, 2, transform, translation, bytes(modified), stream))
    for value in ((0, 0, 0), (17.25, -13.5, 2.75), (2048, 2048, 32768), (2048, 2048, 40000)):
        # Admission rejects large negative coordinates; these explicit cases
        # remain inside the supplied scalar*128 margin and depth interval.
        cases.append(probe.run('translation-' + str(value), synthetic, 0, transform, fbytes(value), header, stream))
    cases.append(probe.run('large-depth-basis', synthetic, 0,
        transform[:24] + fbytes([16384, 0, 0]), translation, header, stream))
    for scale in (.125, .25, .5, 2):
        cases.append(probe.run('footprint-scale-' + str(scale), synthetic, 0, transform, translation, header, stream, scale))
    attached = dict(obj)
    attached['locations'] = b''.join(add(obj['locations'][i:i + 12], fbytes([-400, 200, 0]))
                                    for i in range(0, len(obj['locations']), 12))
    cases.append(probe.run('installed-explicit-attachment', attached, 0, transform, translation, header, stream))
    rejected = []
    for name, change, expected_error in [('hidden-frame', {'obj': dict(synthetic, flags=b'\x80\0\0')}, 'hidden frame'),
            ('bad-frame', {'frame': 3}, 'frame outside'), ('nonfinite-transform', {'transform': words(0x7f800000) + transform[4:]}, 'nonfinite'),
            ('basis-width', {'transform': fbytes([1000] * 9)}, 'destination width'),
            ('origin-width', {'translation': fbytes([1e8, 1e8, 1e8])}, 'origin outside'),
            ('negative-zero', {'translation': words(0x80000000, 0, 0)}, 'negative zero'),
            ('short-translation', {'translation': bytes(8)}, 'shape'),
            ('short-matrix-array', {'obj': dict(synthetic, matrices=synthetic['matrices'][:-4])}, 'frame array extent'),
            ('unsupported-flags', {'header': words(10) + header[4:]}, 'flags-8'),
            ('zero-scalar', {'header': header[:8] + words(0) + header[12:]}, 'positive scalar'),
            ('subnormal-store', {'transform': words(1) + words(0) * 8}, 'nonnormal'),
            ('inexact-extended', {'transform': fbytes([1, 2 ** -80, 1]) + transform[12:]}, 'inexact extended'),
            ('zero-scale', {'scale': 0}, 'positive admission scale')]:
        arguments = dict(obj=synthetic, frame=0, transform=transform, translation=translation, header=header, stream=stream)
        arguments.update(change)
        try:
            probe.run('rejected-' + name, **arguments)
        except RuntimeError as error:
            require(expected_error in str(error), 'wrong domain rejection: ' + str(error))
            rejected.append({'name': name, 'error': str(error)})
        else:
            raise RuntimeError('domain rejection failed: ' + name)
    controls = []
    for mutation in ('reverse-product', 'transpose', 'omit-scalar', 'omit-translation', 'omit-header-vector'):
        try:
            probe.run('negative-' + mutation, synthetic, 0, transform, translation, header, stream, mutation=mutation)
        except RuntimeError as error:
            require('stage oracle mismatch' in str(error), 'wrong oracle control failure: ' + str(error))
            controls.append({'name': mutation, 'error': str(error)})
        else:
            raise RuntimeError('oracle control did not fail: ' + mutation)
    for phase in ('A', 'B'):
        for deny, expected_error in (('code', 'denied matrix'), ('read', 'unexpected read'), ('write', 'unexpected write'), ('budget', 'budget')):
            try:
                probe.run('negative-' + phase + '-' + deny, synthetic, 0, transform, translation, header, stream,
                          deny=deny, deny_phase=phase, budget=1 if deny == 'budget' else 4000)
            except RuntimeError as error:
                require(expected_error in str(error), 'wrong boundary control failure: ' + str(error))
                controls.append({'name': phase + '-' + deny, 'error': str(error)})
            else:
                raise RuntimeError('boundary control did not fail')
    try:
        probe.run('negative-original-admission', synthetic, 0, transform, fbytes([200000, 200000, 1000]), header, stream)
    except RuntimeError as error:
        require(str(error) == 'unexpected code 0x654a03', 'wrong admission boundary failure: ' + str(error))
        controls.append({'name': 'outside-original-admission', 'error': str(error)})
    else:
        raise RuntimeError('original admission did not leave permitted fragment')
    report = {'schema': 1, 'executable_sha256': EXE_HASH, 'probe_sha256': sha(Path(__file__).read_bytes()),
        'imports_sha256': {name: sha(Path(__file__).with_name(name).read_bytes()) for name in
                           ('probe_scout_composition.py', 'probe_cvr_pixels.py', 'probe_cvr_basis.py')},
        'reference': metadata, 'unicorn': unicorn.__version__, 'capstone': capstone.__version__,
        'code_sha256': {f'{a:#x}..{b:#x}': sha(probe.image.read_disk(a, b - a)) for a, b in A_CODE + B_CODE},
        'boundary': 'A stops before lighting; B is separately seeded with captured A arguments. No camera defaults or pixels.',
        'cases': cases, 'domain_rejections': rejected, 'negative_controls': controls}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'cases': len(cases), 'domain_rejections': len(rejected), 'negative_controls': len(controls),
        'instructions': sum(c[p]['instructions'] for c in cases for p in ('A', 'B')),
        'raster_arithmetic_admitted': sum(c['raster_arithmetic_policy']['admissible'] for c in cases),
        'report_sha256': sha(args.report.read_bytes())}))


if __name__ == '__main__':
    main()
