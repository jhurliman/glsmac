"""Execute a pinned SMAC record-to-indexed-pixel kernel with explicit supplied state.

No original loader, camera, light-table builder, RGB palette or OS code executes.
The JSON report contains local executable-derived bytes; keep it outside Git.
"""
from collections import Counter
from itertools import product
from pathlib import Path
import argparse
import hashlib
import json
import struct

import capstone
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
from unicorn.x86_const import (UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW)
from probe_scout_composition import Probe as Image, EXE_HASH, ASSETS, chunks, require, words, within

SELECT = (0x6558d0, 0x655982)
STEPS = (0x65b3a0, 0x65b8f2)
INITIALIZE = (0x686d51, 0x686e43)
# Only the non-block, un-clipped paths, plus the common epilogue, may execute.
DRAW = [(0x6867f0, 0x68681c), (0x686bf8, 0x686d51), (0x686a47, 0x686a52)]
PATCHES = (0x6869ce, 0x6869da, 0x686bbb, 0x686c46, 0x686d11,
    0x686c60, 0x686d25, 0x6869be, 0x6869c8, 0x686bce, 0x686bd8,
    0x686c6a, 0x686c74, 0x686d2f, 0x686d39, 0x6869e5, 0x6869ef,
    0x686c82, 0x686c8c, 0x686a15, 0x686a1f, 0x686cb2, 0x686cbc,
    0x6869fd, 0x686a07, 0x686c9a, 0x686ca4, 0x6869d4, 0x686bde,
    0x686c7a, 0x686d3f, 0x6869f5, 0x686c92, 0x686a25, 0x686cc2,
    0x686a0d, 0x686caa, 0x68699e)
STACK, STOP, DATA = 0x70000000, 0x71000000, 0x72000000
CONTEXT, SURFACE, ROWS = DATA, DATA + 0x100, DATA + 0x1000
SHADE, TABLE, STREAM = DATA + 0x2000, DATA + 0x3000, DATA + 0x5000
COLOR, DEPTH = DATA + 0x100000, DATA + 0x200000
SAVED = {UC_X86_REG_EBX: 0x11111111, UC_X86_REG_ESI: 0x22222222,
         UC_X86_REG_EDI: 0x33333333, UC_X86_REG_EBP: 0x44444444}
COEFFICIENTS = [p for p in product((-1, 0, 1), repeat=3) if p != (0, 0, 0)]


def hash_bytes(value):
    return hashlib.sha256(value).hexdigest()


def record(code, lighting, color):
    require(0 <= code < 26 and 0 <= lighting < 2048 and 0 <= color < 256, 'bad synthetic record')
    return bytes([(code << 3) | (lighting >> 8), lighting & 255, color])


def installed_stream(directory, filename, object_name):
    """Extract one flags-8 simple stream; do not emulate the original loader."""
    raw = (directory / filename).read_bytes()
    require(hash_bytes(raw) == ASSETS[filename][0], 'asset hash mismatch')
    require(raw[:4] == b'CVR\0' and int.from_bytes(raw[4:8], 'little') == len(raw), 'bad CVR envelope')
    outer = list(chunks(raw[8:]))
    require([t for t, _ in outer] == [1, 2, 3, 4], 'unexpected CVR envelope fields')
    fields = list(chunks(outer[3][1]))
    require([t for t, _ in fields[:3]] == [0x104, 0x204, 0x304], 'unexpected scene fields')
    matches = []
    for tag, value in fields[3:]:
        require(tag == 0x404, 'unexpected object field')
        obj = list(chunks(value))
        if obj[0] != (0x10404, object_name.encode('ascii')):
            continue
        require([t for t, _ in obj] == [0x10404, 0x20404, 0x30404], 'unexpected selected object fields')
        geometry = list(chunks(obj[1][1]))
        require(len(geometry) == 1 and geometry[0][0] == 0x1020404, 'unexpected geometry container')
        data = geometry[0][1]
        require(len(data) >= 47, 'short geometry')
        flags, size, scalar, end, end_minus_one = struct.unpack_from('<5I', data)
        count = struct.unpack_from('<I', data, 38)[0]
        require(flags == 8 and size == end == len(data) and end_minus_one == size - 1, 'not a bounded flags-8 profile')
        require(count > 1 and len(data) == 47 + count * 3 and data[-5:] == bytes(5), 'unexpected record extent')
        stream = data[42:-5]
        require(all((stream[i] >> 3) < 26 for i in range(0, len(stream) - 3, 3)) and stream[-3] >> 3 == 26, 'unexpected direction/terminal')
        matches.append((stream, {'file': filename, 'file_sha256': hash_bytes(raw), 'object': object_name,
                        'geometry_sha256': hash_bytes(data), 'flags': flags, 'scalar_bits': hex(scalar),
                        'raw_header': data[:42].hex(), 'record_count': count}))
    require(len(matches) == 1, 'selected object must be unique')
    return matches[0]


def fit_stream(stream):
    """Provide a bounded synthetic orthogonal projection; not original camera state."""
    position = [0, 0, 0]
    minimum, maximum = [0, 0, 0], [0, 0, 0]
    for i in range(0, len(stream) - 3, 3):
        position = [a + b for a, b in zip(position, COEFFICIENTS[stream[i] >> 3])]
        minimum = [min(a, b) for a, b in zip(minimum, position)]
        maximum = [max(a, b) for a, b in zip(maximum, position)]
    width, height = [maximum[i] - minimum[i] + 5 for i in (0, 1)]
    return {'basis': ((256, 0, 0), (0, 256, 0), (0, 0, 256)),
            'initial': ((2 - minimum[0]) * 256, (2 - minimum[1]) * 256, (1024 - minimum[2]) * 256),
            'width': width, 'height': height, 'stride': width + 7}


def oracle(stream, mode, basis, initial, width, height, stride, shade, table, color, depth,
           mutation=None):
    """Independent scalar interpretation; no emulator state is read here."""
    a, b, d = initial
    pixels, depths = bytearray(color), bytearray(depth)
    samples, written = [], []
    effective = len(stream) // 3 - bool(mode & 0x20)
    for index in range(effective):
        first, second, third = stream[index * 3:index * 3 + 3]
        coefficient = COEFFICIENTS[first >> 3]
        delta = [sum(coefficient[j] * basis[i][j] for j in range(3)) for i in range(3)]
        if mutation == 'wrong-step':
            delta[0] = -delta[0]
        a, b, d = a + delta[0], b + delta[1], d + delta[2]
        require(all(-(1 << 23) <= v < (1 << 23) for v in (a, b, d)), 'oracle excludes accumulator overflow')
        x, y, z = a // 256, b // 256, (d // 256) & 0xffff
        lighting = ((first & 7) << 8) | second
        color_index = table[shade[lighting] * 256 + third]
        if mutation == 'ignore-light':
            color_index = table[third]
        covered = [(0, 0)] if mode & 1 else [(0, 0), (1, 0), (0, 1), (1, 1)]
        samples.append({'record': index, 'code': first >> 3, 'coefficient': coefficient,
                        'lighting_index': lighting, 'material_index': third,
                        'color_index': color_index, 'sample': [x, y, z]})
        for dx, dy in covered:
            require(0 <= x + dx < width and 0 <= y + dy < height, 'fixture outside un-clipped output')
            offset = (y + dy) * stride + x + dx
            old = struct.unpack_from('<H', depths, offset * 2)[0]
            accepted = z <= old if mutation == 'non-strict-depth' else z < old
            if accepted:
                struct.pack_into('<H', depths, offset * 2, z)
                pixels[offset] = color_index
                written.append([offset, z, color_index])
    return bytes(pixels), bytes(depths), samples, written


class PixelProbe:
    def __init__(self, executable):
        self.image = Image(executable)
        dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        dis.detail = True
        self.instructions = {}
        for start, end in [SELECT, STEPS, INITIALIZE, *DRAW]:
            parsed = list(dis.disasm(self.image.read_disk(start, end - start), start))
            require(parsed and parsed[-1].address + parsed[-1].size == end, 'range ends mid-instruction')
            self.instructions.update({ins.address: ins for ins in parsed})
        # Every allowed code write must be an existing displacement/immediate field.
        kernel = list(dis.disasm(self.image.read_disk(0x6867f0, 0x686d51 - 0x6867f0), 0x6867f0))
        operand_fields = {ins.address + ins.disp_offset for ins in kernel if ins.disp_size == 4}
        operand_fields |= {ins.address + ins.imm_offset for ins in kernel if ins.imm_size == 4}
        require(all(p in operand_fields for p in PATCHES), 'patch is not an original operand')
        require(all(self.image.read_disk(p, 4) == words(0x01020304) for p in PATCHES), 'unexpected patch placeholder')

    def run(self, name, stream, mode=1, basis=((256, 0, 128), (0, 256, -128), (512, -256, 768)),
            initial=(8192, 8192, 8192), width=64, height=64, stride=80,
            initial_depth=65535, deny=None, mutation=None, budget=500000):
        require(mode in (0, 1, 32, 33) and len(stream) % 3 == 0 and len(stream) // 3 > bool(mode & 32), 'unsupported fixture mode/count')
        require(len(stream) < 0xe0000 and 0 < width <= stride and 0 < height <= 1024
                and stride * height < 0x70000, 'fixture too large or overlapping')
        require(all(-32768 <= v <= 32767 for row in basis[:2] for v in row), 'basis not signed16')
        shade = bytes((i * 13 + 7) % 24 for i in range(2048))
        table = bytes((s * 17 + c * 7 + 3) & 255 for s in range(24) for c in range(256))
        color = b'\xa5' * (stride * height)
        depth = struct.pack('<H', initial_depth) * (stride * height)
        expected_color, expected_depth, samples, expected_writes = oracle(
            stream, mode, basis, initial, width, height, stride, shade, table, color, depth, mutation)
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        image = self.image
        uc.mem_map(image.base, (image.size + 4095) & ~4095)
        uc.mem_write(image.base, image.raw[:image.header])
        for start, _, length, offset in image.sections:
            uc.mem_write(start, image.raw[offset:offset + length])
        for address, size in [(STACK, 0x10000), (STOP, 0x1000), (DATA, 0x300000)]:
            uc.mem_map(address, size)
        uc.mem_write(STACK, b'\xa5' * 0x10000)
        supplied = []

        def supply(address, value, label):
            uc.mem_write(address, value)
            supplied.append({'address': hex(address), 'label': label, 'bytes': value.hex()})

        supply(CONTEXT, words(0, SURFACE) + bytes(0x78), 'explicit flags-zero context')
        supply(SURFACE, bytes(32) + b'\x01', 'explicit one-byte surface format')
        supply(ROWS, words(*(i * stride for i in range(height))), 'explicit row-offset table')
        supply(SHADE, shade, 'synthetic 11-bit lighting-index lookup, not original lighting')
        supply(TABLE, table, 'synthetic lighting/material lookup, not original palette')
        supply(STREAM, stream, 'raw record stream')
        supply(COLOR - 16, b'\x5a' * 16 + color + b'\x5a' * 16, 'color payload and canaries')
        supply(DEPTH - 16, b'\x5a' * 16 + depth + b'\x5a' * 16, 'depth payload and canaries')
        for address, value in [(0x9c6b94, SHADE), (0x9c6b98, DEPTH), (0x9c6b9c, COLOR),
                               (0x9c6ba4, stride), (0x9c6ba8, stride), (0x9c6bb0, TABLE), (0x9c8d1c, ROWS)]:
            supply(address, words(value), 'explicit raster global')
        supply(0x9c8f40, struct.pack('<6h3i', *basis[0], *basis[1], *basis[2]), 'explicit projected basis; no camera inference')
        # Only these three words per stride and the helper's actual scratch outputs are accessible.
        step_ranges = [(0x9c8d40 + i * 16, 0x9c8d4c + i * 16) for i in range(26)]
        packed_ranges = [(0x9c9180 + i * 16, 0x9c918c + i * 16) for i in range(26)]
        helper_scratch = [(0x9c8ee4, 0x9c8f0c)]
        stack_range = [(STACK + 0xfe00, STACK + 0xff40)]
        phases, execution, accesses, actual_pixel_writes, actual_buffer_writes = [], Counter(), [], [], []
        active_code, reads, writes = [], [], []
        instruction_count, phase_count, finished, phase = 0, 0, False, ''

        def code_hook(machine, address, size, _):
            nonlocal instruction_count, phase_count, finished
            if address == STOP:
                finished = True
                machine.emu_stop()
                return
            require(address in self.instructions and within(address, size, active_code), f'unexpected code {address:#x} in {phase}')
            if deny == 'code' and phase == 'draw' and address == 0x686cdc:
                raise RuntimeError('control denied draw code')
            instruction_count += 1
            phase_count += 1
            require(instruction_count <= budget, 'instruction budget exceeded')
            execution[(phase, address)] += 1

        def memory_hook(machine, kind, address, size, value, _):
            is_read = kind == UC_MEM_READ
            label, ranges = ('read', reads) if is_read else ('write', writes)
            require(within(address, size, ranges), f'unexpected {label} {address:#x}+{size} in {phase} at {machine.reg_read(UC_X86_REG_EIP):#x}')
            actual = bytes(machine.mem_read(address, size)).hex() if is_read else (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()
            accesses.append({'phase': phase, 'instruction': hex(machine.reg_read(UC_X86_REG_EIP)), 'kind': label,
                             'address': hex(address), 'size': size, 'value': actual})
            if not is_read and phase == 'draw' and COLOR <= address < COLOR + len(color):
                actual_pixel_writes.append([address - COLOR, value])
            if not is_read and phase == 'draw' and (COLOR <= address < COLOR + len(color) or DEPTH <= address < DEPTH + len(depth)):
                actual_buffer_writes.append([address, size, value])

        uc.hook_add(UC_HOOK_CODE, code_hook)
        uc.hook_add(UC_HOOK_MEM_READ, memory_hook)
        uc.hook_add(UC_HOOK_MEM_WRITE, memory_hook)

        def invoke(label, entry, args, code, allowed_reads, allowed_writes):
            nonlocal phase, active_code, reads, writes, phase_count, finished
            phase, active_code = label, code
            reads, writes = stack_range + allowed_reads, stack_range + allowed_writes
            phase_count, finished = 0, False
            sp = STACK + 0xff00
            supply(sp, words(STOP, *args), label + ' cdecl arguments')
            uc.reg_write(UC_X86_REG_ESP, sp)
            uc.reg_write(UC_X86_REG_EFLAGS, 0x202)
            uc.reg_write(UC_X86_REG_FPCW, 0x037f)
            for register, value in SAVED.items():
                uc.reg_write(register, value)
            uc.emu_start(entry, STOP + 1, timeout=2_000_000, count=budget + 1)
            require(finished, 'return sentinel not reached')
            require(uc.reg_read(UC_X86_REG_ESP) == sp + 4, 'wrong cdecl stack cleanup')
            require(all(uc.reg_read(r) == v for r, v in SAVED.items()), 'callee-saved register changed')
            phases.append({'name': label, 'entry': hex(entry), 'instructions': phase_count,
                           'allowed_code': [[hex(a), hex(b)] for a, b in code],
                           'allowed_reads': [[hex(a), hex(b)] for a, b in reads],
                           'allowed_writes': [[hex(a), hex(b)] for a, b in writes]})

        invoke('select', SELECT[0], [CONTEXT], [SELECT], [(CONTEXT, CONTEXT + 8), (SURFACE + 32, SURFACE + 33)],
               [(CONTEXT + 0x6c, CONTEXT + 0x80)])
        selected = struct.unpack('<5I', uc.mem_read(CONTEXT + 0x6c, 20))
        require(selected == (0x6867f0, 0x686000, 0x688fd0, 0x688a80, 0x655a70), 'unexpected original dispatch')
        invoke('steps', STEPS[0], [], [STEPS], [(0x9c8f40, 0x9c8f58), (0x9c6ba8, 0x9c6bac)] + step_ranges,
               step_ranges + packed_ranges + helper_scratch)
        expected_steps = [[sum(c[j] * basis[i][j] for j in range(3)) * (256 if i == 2 else 1)
                           for i in range(3)] for c in COEFFICIENTS]
        actual_steps = [list(struct.unpack('<3i', uc.mem_read(a, 12))) for a, _ in step_ranges]
        require(actual_steps == expected_steps, '26-step coefficient mismatch')
        init_reads = [(a, a + 4) for a in (0x9c6bb0, 0x9c8d1c, 0x9c6b98, 0x9c6b9c, 0x9c6ba4, 0x9c6ba8)]
        patch_writes = [(p, p + 4) for p in PATCHES if deny != 'patch' or p != 0x686d11]
        invoke('initialize', INITIALIZE[0], [], [INITIALIZE], init_reads, patch_writes)
        patched = bytes(uc.mem_read(0x6867f0, 0x686d51 - 0x6867f0))
        original = image.read_disk(0x6867f0, len(patched))
        require(all(a == b or within(0x6867f0 + i, 1, [(p, p + 4) for p in PATCHES])
                    for i, (a, b) in enumerate(zip(original, patched))), 'initializer changed non-operand code')
        require({int(x['address'], 16) for x in accesses if x['phase'] == 'initialize' and x['kind'] == 'write'} == set(PATCHES), 'incomplete patch set')
        require(sum(x['phase'] == 'initialize' and x['kind'] == 'write' for x in accesses) == len(PATCHES), 'repeated patch write')
        draw_reads = [(STREAM, STREAM + len(samples) * 3), (SHADE, SHADE + len(shade)), (TABLE, TABLE + len(table)),
                      (ROWS, ROWS + height * 4), (DEPTH, DEPTH + len(depth)), (0x9c6b94, 0x9c6b98), (0x9c8f18, 0x9c8f1c)] + step_ranges
        if deny == 'read':
            draw_reads.remove((SHADE, SHADE + len(shade)))
        draw_writes = [(0x9c8f18, 0x9c8f1c), (DEPTH, DEPTH + len(depth))]
        if deny != 'write':
            draw_writes.append((COLOR, COLOR + len(color)))
        invoke('draw', selected[0], [*(v & 0xffffffff for v in initial), len(stream) // 3, STREAM, mode], DRAW, draw_reads, draw_writes)
        record_reads = [[int(x['address'], 16), x['size']] for x in accesses
                        if x['phase'] == 'draw' and x['kind'] == 'read'
                        and STREAM <= int(x['address'], 16) < STREAM + len(stream)]
        require(record_reads == [[STREAM + i, 1] for i in range(len(samples) * 3)], 'record read order/count mismatch')
        require(bytes(uc.mem_read(COLOR, len(color))) == expected_color, 'color payload oracle mismatch')
        require(bytes(uc.mem_read(DEPTH, len(depth))) == expected_depth, 'depth payload oracle mismatch')
        require(actual_pixel_writes == [[offset, color_value] for offset, _, color_value in expected_writes], 'ordered pixel writes mismatch')
        require(actual_buffer_writes == [item for offset, z, value in expected_writes
                    for item in ([DEPTH + offset * 2, 2, z], [COLOR + offset, 1, value])], 'ordered depth/color writes mismatch')
        for address, length in [(COLOR, len(color)), (DEPTH, len(depth))]:
            require(bytes(uc.mem_read(address - 16, 16)) == b'\x5a' * 16 and bytes(uc.mem_read(address + length, 16)) == b'\x5a' * 16, 'output canary changed')
        require(bytes(uc.mem_read(STACK, 0xfe00)) == b'\xa5' * 0xfe00, 'stack lower canary changed')
        require(bytes(uc.mem_read(STACK + 0xff40, 0xc0)) == b'\xa5' * 0xc0, 'stack upper canary changed')
        return {'name': name, 'mode': mode, 'basis': basis, 'initial': initial, 'size': [width, height, stride],
                'initial_depth': initial_depth, 'instructions': instruction_count, 'phases': phases,
                'source_records': len(stream) // 3, 'drawn_records': len(samples), 'samples': samples,
                'written_samples': expected_writes, 'color_sha256': hash_bytes(expected_color), 'depth_sha256': hash_bytes(expected_depth),
                'steps': actual_steps, 'code_before_sha256': hash_bytes(original), 'code_after_sha256': hash_bytes(patched),
                'patched_operands': [{'address': hex(p), 'value': bytes(uc.mem_read(p, 4)).hex()} for p in PATCHES],
                'supplied_bytes': supplied, 'memory_trace': accesses,
                'executed_instructions': [{'phase': p, 'address': hex(a), 'count': n} for (p, a), n in sorted(execution.items())]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    probe = PixelProbe(args.exe)
    cases = []
    for code in range(26):
        cases.append(probe.run('direction-' + str(code), record(code, code * 79 % 2048, code * 11 % 256)))
    stream = b''.join(record(c, (c * 79 + 257) % 2048, c * 9 % 256) for c in range(26))
    for mode in (0, 1, 32, 33):
        terminal = b'\xd7\xff\xff' if mode & 32 else b''
        cases.append(probe.run('ordered-mode-' + str(mode), stream + terminal, mode))
    for depth in (0, 31, 32, 33, 65535):
        cases.append(probe.run('depth-' + str(depth), record(13, 2047, 255), basis=((0, 0, 0), (0, 0, 0), (0, 0, 0)), initial_depth=depth))
    cases.append(probe.run('repeated-same-depth', record(13, 0, 1) + record(13, 2047, 255), basis=((0, 0, 0), (0, 0, 0), (0, 0, 0))))
    cases.append(probe.run('fractional-floor', record(13, 1, 2), basis=((0, 0, -129), (0, 0, 127), (0, 0, -257)), initial=(8192, 8192, 8192)))
    cases.append(probe.run('negative-depth-unsigned-tie', record(13, 1, 2), basis=((0, 0, 0), (0, 0, 0), (0, 0, 0)), initial=(8192, 8192, -256)))
    for filename, object_name in [('vi.cvr', 'EH_M1-Mask05'), ('vipta00.cvr', 'VIPTA00')]:
        installed, metadata = installed_stream(args.assets, filename, object_name)
        for mode in (32, 33):
            result = probe.run(filename + '-mode-' + str(mode), installed, mode, **fit_stream(installed))
            result['installed_geometry'] = metadata
            cases.append(result)
    controls = []
    for field, value, expected in [('deny', 'code', 'control denied'), ('deny', 'read', 'unexpected read'),
            ('deny', 'write', 'unexpected write'), ('deny', 'patch', 'unexpected write'),
            ('mutation', 'wrong-step', 'color payload oracle mismatch'), ('mutation', 'ignore-light', 'color payload oracle mismatch'),
            ('mutation', 'non-strict-depth', 'color payload oracle mismatch'), ('budget', 1, 'instruction budget exceeded')]:
        kwargs = {field: value}
        if value == 'non-strict-depth':
            kwargs.update(initial_depth=35)
        try:
            probe.run('negative-' + str(value), record(13, 2047, 255), **kwargs)
        except RuntimeError as error:
            require(expected in str(error), 'control failed for unintended reason: ' + str(error))
            controls.append({'control': value, 'rejected': str(error)})
        else:
            raise RuntimeError('control unexpectedly passed: ' + str(value))
    report = {'executable_sha256': EXE_HASH, 'probe_sha256': hash_bytes(Path(__file__).read_bytes()),
              'helper_sha256': hash_bytes(Path(__file__).with_name('probe_scout_composition.py').read_bytes()),
              'capstone_version': capstone.__version__, 'unicorn_version': unicorn.__version__,
              'scope': 'Original un-clipped indexed raster kernel, original selector, step builder and operand initializer; supplied basis/tables/surface. No RGB/camera/loader/OS.',
              'cases': cases, 'negative_controls': controls}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'cases': len(cases), 'controls': len(controls), 'report_sha256': hash_bytes(args.report.read_bytes())}))


if __name__ == '__main__':
    main()
