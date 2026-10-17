"""Execute pinned SMAC scene assembly, without loader, geometry or rasterization."""
from collections import Counter
from pathlib import Path
import argparse
import hashlib
import json
import struct

import capstone
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS, UC_X86_REG_FPCW)

EXE_HASH = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
ASSETS = {
    'vi.cvr': ('d2044c1f77a0e0377bc79c2dcbedc7fc3813d9c5e434a042a0e52fc9f0b779f1', 0x7af6c8),
    'vipta00.cvr': ('b54fff2c0096e1545004c15fb2fd546883a45aa0af96ffe58d9daa5054ededbd', 0x7af808),
}
SELECTORS = [(0x4d8700, 0x4d87e0), (0x4d87e0, 0x4d88e0), (0x4d88e0, 0x4d8df4),
             (0x4d8e40, 0x4d9118), (0x4d9130, 0x4d959c)]
COMPOSITION = [(0x631a60, 0x631bdf), (0x641ee0, 0x641f05)]
TABLES = [(0x4d8df4, 0x4d8e40), (0x4d9118, 0x4d9130), (0x4d959c, 0x4d95e8)]
STACK, OUTPUT, STOP, DATA, DEST = 0x70000000, 0x71000000, 0x72000000, 0x73000000, 0x74000000
SAVED = {UC_X86_REG_EBX: 0x11111111, UC_X86_REG_ESI: 0x22222222,
         UC_X86_REG_EDI: 0x33333333, UC_X86_REG_EBP: 0x44444444}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def words(*values):
    return struct.pack('<' + 'I' * len(values), *values)


def within(address, size, ranges):
    return any(start <= address and address + size <= end for start, end in ranges)


def chunks(raw):
    cursor = 0
    while cursor < len(raw):
        require(len(raw) - cursor >= 8, 'short chunk header')
        tag = int.from_bytes(raw[cursor:cursor + 4], 'big')
        size = int.from_bytes(raw[cursor + 4:cursor + 8], 'little')
        require(8 <= size <= len(raw) - cursor, 'bad chunk extent')
        yield tag, raw[cursor + 8:cursor + size]
        cursor += size


def scene(path, expected_hash):
    raw = path.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == expected_hash, 'asset hash mismatch: ' + str(path))
    require(raw[:4] == b'CVR\0' and int.from_bytes(raw[4:8], 'little') == len(raw), 'bad envelope')
    outer = list(chunks(raw[8:]))
    require([tag for tag, _ in outer] == [1, 2, 3, 4], 'unexpected outer fields')
    fields = list(chunks(outer[3][1]))
    require([tag for tag, _ in fields[:3]] == [0x104, 0x204, 0x304], 'unexpected scene fields')
    count, frames = (int.from_bytes(fields[i][1], 'little') for i in (1, 2))
    require(frames > 0 and len(fields) == count + 3, 'bad scene counts')
    result = []
    for tag, raw_object in fields[3:]:
        require(tag == 0x404, 'unexpected object field')
        obj = list(chunks(raw_object))
        require(obj[0][0] == 0x10404 and obj[-1][0] == 0x30404, 'unexpected object order')
        require([t for t, _ in obj] in ([0x10404, 0x30404], [0x10404, 0x20404, 0x30404]), 'unexpected object schema')
        animation = list(chunks(obj[-1][1]))
        require([t for t, _ in animation] == [0x1030404, 0x2030404, 0x3030404], 'unexpected animation fields')
        arrays = [value for _, value in animation]
        require([len(value) for value in arrays] == [frames, 12 * frames, 36 * frames], 'bad array extents')
        result.append({'name': obj[0][1].decode('cp1252'), 'flags': arrays[0], 'locations': arrays[1], 'matrices': arrays[2]})
    return {'file': path.name, 'sha256': expected_hash, 'frames': frames, 'objects': result}


def synthetic_scene():
    objects = []
    for index in range(2):
        locations = [(index * 10 + frame + .25, -frame - .5, index - frame * 2.0) for frame in range(3)]
        matrices = [[index * 100 + frame * 10 + cell + .125 for cell in range(9)] for frame in range(3)]
        objects.append({'name': 'synthetic-' + str(index), 'flags': bytes([0, 128, 255] if index == 0 else [1, 255, 0]),
                        'locations': b''.join(struct.pack('<3f', *value) for value in locations),
                        'matrices': b''.join(struct.pack('<9f', *value) for value in matrices)})
    return {'file': 'synthetic', 'frames': 3, 'objects': objects}


class Probe:
    def __init__(self, executable):
        self.raw = executable.read_bytes()
        require(hashlib.sha256(self.raw).hexdigest() == EXE_HASH, 'executable hash mismatch')
        pe = struct.unpack_from('<I', self.raw, 0x3c)[0]
        require(self.raw[pe:pe + 4] == b'PE\0\0' and struct.unpack_from('<H', self.raw, pe + 24)[0] == 0x10b, 'not PE32')
        self.base = struct.unpack_from('<I', self.raw, pe + 52)[0]
        self.size = struct.unpack_from('<I', self.raw, pe + 80)[0]
        self.header = struct.unpack_from('<I', self.raw, pe + 84)[0]
        self.sections = []
        count, optional = struct.unpack_from('<H', self.raw, pe + 6)[0], struct.unpack_from('<H', self.raw, pe + 20)[0]
        for i in range(count):
            size, rva, length, offset = struct.unpack_from('<IIII', self.raw, pe + 24 + optional + i * 40 + 8)
            self.sections.append((self.base + rva, size, length, offset))
        self.offset = self.read_disk(0x6aa028, 12)
        require(self.offset == words(0xc3c80000, 0x43480000, 0), 'original component offset changed')

    def read_disk(self, address, size):
        for start, _, length, offset in self.sections:
            if start <= address and address + size <= start + length:
                return self.raw[offset + address - start:offset + address - start + size]
        raise RuntimeError('no disk bytes for ' + hex(address))

    def run(self, name, scenes, frame=0, wrapper=False, offset=None, initial=0,
            budget=20000, deny=None, oracle_mutation=None):
        require(0 <= initial <= 200 and frame >= 0, 'probe supplies only bounded counts/nonnegative frames')
        machine = Uc(UC_ARCH_X86, UC_MODE_32)
        machine.mem_map(self.base, (self.size + 4095) & ~4095)
        machine.mem_write(self.base, self.raw[:self.header])
        for start, _, length, disk_offset in self.sections:
            machine.mem_write(start, self.raw[disk_offset:disk_offset + length])
        for address, size in [(STACK, 0x10000), (OUTPUT, 0x2000), (STOP, 0x1000), (DATA, 0x100000), (DEST, 0x10000)]:
            machine.mem_map(address, size)
        machine.mem_write(OUTPUT, b'\xa5' * 0x2000)
        machine.mem_write(DEST, b'\xa5' * 0x10000)
        reads, writes, supplied = [(STACK, STACK + 0x10000)], [(STACK, STACK + 0x10000)], []
        cursor = DATA

        def supply(address, value, reason, readable=True):
            machine.mem_write(address, value)
            supplied.append({'address': hex(address), 'bytes': value.hex(), 'reason': reason})
            if readable:
                reads.append((address, address + len(value)))

        def allocate(value, reason):
            nonlocal cursor
            address = cursor
            cursor += (len(value) + 15) & ~15
            require(cursor <= DATA + 0x100000, 'synthetic arena exhausted')
            supply(address, value, reason)
            return address

        expected_output = bytearray(b'\xa5' * 0x2000)
        expected_dest = bytearray(b'\xa5' * 0x10000)
        for index in range(200):
            pointers = words(DEST + index * 64, DEST + index * 64 + 4, DEST + index * 64 + 16)
            supply(OUTPUT + 0x74c + index * 12, pointers, 'caller-owned output pointer triple')
            expected_output[0x74c + index * 12:0x758 + index * 12] = pointers
            writes.extend([(DEST + index * 64, DEST + index * 64 + 1),
                           (DEST + index * 64 + 4, DEST + index * 64 + 16),
                           (DEST + index * 64 + 16, DEST + index * 64 + 52)])
        supply(OUTPUT + 0x13cc, words(initial), 'initial append count')
        expected_output[0x13cc:0x13d0] = words(initial)
        writes.extend([(OUTPUT + 0x13cc, OUTPUT + 0x13d0), (OUTPUT + 0x10c, OUTPUT + 0x74c),
                       (OUTPUT + 0x10ac, OUTPUT + 0x13cc)])
        model_info = []
        for scene_index, source in enumerate(scenes):
            triples, geometry = [], []
            for object_index, obj in enumerate(source['objects']):
                triples.extend(allocate(obj[field], f"{source['file']} object {object_index} {field}")
                               for field in ('flags', 'locations', 'matrices'))
                # Composition copies these two words without dereferencing them.
                geometry.extend([0x55000000 + scene_index * 0x10000 + object_index,
                                 0x56000000 + scene_index * 0x10000 + object_index * 16])
            arrays = allocate(words(*triples), 'loaded animation pointer triples')
            descriptors = allocate(words(*geometry), 'synthetic opaque geometry descriptor pairs')
            loaded = bytearray(0x44)
            table_base = 0x57000000 + scene_index * 0x10000
            for field, value in [(0x14, table_base), (0x20, source['frames']), (0x24, len(source['objects'])), (0x2c, descriptors), (0x30, arrays)]:
                loaded[field:field + 4] = words(value)
            loaded_address = allocate(bytes(loaded), 'synthetic loaded-scene descriptor; unused fields zero')
            model = ASSETS[source['file'].lower()][1] if source['file'].lower() in ASSETS else allocate(bytes(12), 'synthetic model handle')
            supply(model + 8, words(loaded_address), 'model loaded-scene pointer')
            model_info.append((model, geometry, table_base))
        translation = self.offset if offset is None else struct.pack('<3f', *offset)
        translation_address = 0x6aa028 if offset is None else allocate(translation, 'explicit synthetic translation')
        if offset is None:
            reads.append((0x6aa028, 0x6aa034))
        if wrapper:
            require(initial == 0 and offset is None and len(scenes) in (1, 2), 'unsupported selector fixture')
            reads.extend(TABLES)
            supply(0x9b6fb0, b'\x01', 'explicit Gun attack 1, matching pinned table evidence')
            supply(0x9b6fb2, b'\x00', 'explicit Gun mode 0, matching pinned table evidence')
            supply(0x8f3c60, words(frame), 'explicit nonnegative animation counter')
        if deny == 'read':
            reads = [r for r in reads if r != (translation_address, translation_address + 12)]
        if deny == 'write':
            writes = [r for r in writes if r != (DEST, DEST + 1)]
        code = COMPOSITION + (SELECTORS if wrapper else [])
        allowed_starts = set()
        dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        for start, end in code:
            allowed_starts.update(ins.address for ins in dis.disasm(self.read_disk(start, end - start), start))
        if deny == 'code':
            allowed_starts.remove(0x641ee0)
        access, executed, calls = Counter(), Counter(), []
        instructions = 0
        finished = False

        def on_code(uc, address, size, _):
            nonlocal instructions, finished
            if address == STOP:
                finished = True
                uc.emu_stop()
                return
            require(address in allowed_starts and within(address, size, code), f'unexpected code {address:#x}')
            instructions += 1
            require(instructions <= budget, 'instruction budget exceeded')
            executed[address] += 1
            if address == 0x631a60:
                sp = uc.reg_read(UC_X86_REG_ESP)
                require(within(sp, 16, [(STACK, STACK + 0x10000)]), 'composition call stack outside fixture')
                raw = bytes(uc.mem_read(sp, 16))
                return_to, model, location, selected_frame = struct.unpack('<4I', raw)
                calls.append({'entry': hex(address), 'return_to': hex(return_to), 'model': hex(model),
                              'offset_pointer': hex(location), 'frame': selected_frame})

        def on_memory(uc, kind, address, size, value, _):
            label, ranges = ('read', reads) if kind == UC_MEM_READ else ('write', writes)
            require(within(address, size, ranges), f'unexpected {label} {address:#x}+{size} at {uc.reg_read(UC_X86_REG_EIP):#x}')
            access[(label, address, size)] += 1

        machine.hook_add(UC_HOOK_CODE, on_code)
        machine.hook_add(UC_HOOK_MEM_READ, on_memory)
        machine.hook_add(UC_HOOK_MEM_WRITE, on_memory)
        results = []
        invocations = [None] if wrapper else list(range(len(scenes)))
        for invocation in invocations:
            finished = False
            sp = STACK + 0xff00
            args = [1, 0, 0, len(scenes) - 1, 0, 0, OUTPUT] if wrapper else [model_info[invocation][0], translation_address, frame]
            supply(sp, words(STOP, *args), 'return sentinel and explicit call arguments')
            machine.reg_write(UC_X86_REG_ESP, sp)
            machine.reg_write(UC_X86_REG_ECX, OUTPUT)
            machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
            machine.reg_write(UC_X86_REG_FPCW, 0x037f)
            for register, value in SAVED.items():
                machine.reg_write(register, value)
            entry = 0x4d87e0 if wrapper else 0x631a60
            machine.emu_start(entry, STOP + 1, timeout=2_000_000, count=budget + 10)
            require(finished, 'return sentinel not reached within limits')
            require(machine.reg_read(UC_X86_REG_ESP) == sp + (4 if wrapper else 16), 'wrong stack cleanup')
            require(all(machine.reg_read(register) == value for register, value in SAVED.items()), 'callee-saved register changed')
            expected_return = 0
            for source_index in (range(len(scenes)) if wrapper else [invocation]):
                source = scenes[source_index]
                selected = (frame % 15 if wrapper else frame) % source['frames']
                geometry, table_base = model_info[source_index][1:]
                for object_index, obj in enumerate(source['objects']):
                    count = int.from_bytes(expected_output[0x13cc:0x13d0], 'little')
                    if count == 200:
                        expected_return = 1
                        break
                    location = struct.unpack_from('<3f', obj['locations'], selected * 12)
                    delta = struct.unpack('<3f', translation)
                    transformed = struct.pack('<3f', *(a + b for a, b in zip(location, delta)))
                    matrix = obj['matrices'][selected * 36:(selected + 1) * 36]
                    flag = obj['flags'][selected:selected + 1]
                    if oracle_mutation == 'omit-offset':
                        transformed = obj['locations'][selected * 12:(selected + 1) * 12]
                    if oracle_mutation == 'transpose-matrix':
                        values = struct.unpack('<9I', matrix)
                        matrix = words(*(values[row + column * 3] for row in range(3) for column in range(3)))
                    expected_dest[count * 64:count * 64 + 1] = flag
                    expected_dest[count * 64 + 4:count * 64 + 16] = transformed
                    expected_dest[count * 64 + 16:count * 64 + 52] = matrix
                    expected_output[0x10c + count * 8:0x114 + count * 8] = words(*geometry[object_index * 2:object_index * 2 + 2])
                    expected_output[0x10ac + count * 4:0x10b0 + count * 4] = words(table_base + 0x302)
                    expected_output[0x13cc:0x13d0] = words(count + 1)
                    results.append({'source': source['file'], 'object_index': object_index, 'name': obj['name'],
                                    'selected_frame': selected, 'flag': flag.hex(), 'location_bits': transformed.hex(),
                                    'matrix_bits': matrix.hex(), 'output_index': count})
            if not wrapper:
                require(machine.reg_read(UC_X86_REG_EAX) == expected_return, 'unexpected composition return status')
            require(bytes(machine.mem_read(OUTPUT, 0x2000)) == expected_output, 'output descriptor mismatch')
            require(bytes(machine.mem_read(DEST, 0x10000)) == expected_dest, 'output frame-data mismatch')
        expected_calls = [(info[0], translation_address, frame % 15 if wrapper else frame) for info in model_info]
        require([(int(c['model'], 16), int(c['offset_pointer'], 16), c['frame']) for c in calls] == expected_calls, 'unexpected complete composition call order')
        return {'name': name, 'entry': hex(0x4d87e0 if wrapper else 0x631a60), 'frame': frame,
                'wrapper_inputs': {'faction': 1, 'chassis': 0, 'weapon': 0, 'defense': len(scenes) - 1, 'reactor': 0, 'abilities': 0} if wrapper else None,
                'initial_count': initial, 'offset_bits': translation.hex(), 'instruction_count': instructions,
                'final_count': int.from_bytes(expected_output[0x13cc:0x13d0], 'little'),
                'calls': calls, 'objects': results, 'supplied_bytes': supplied,
                'allowed_code': [[hex(a), hex(b)] for a, b in code],
                'allowed_reads': [[hex(a), hex(b)] for a, b in reads],
                'allowed_writes': [[hex(a), hex(b)] for a, b in writes],
                'executed_instructions': [{'address': hex(a), 'count': n} for a, n in sorted(executed.items())],
                'memory_accesses': [{'kind': k, 'address': hex(a), 'size': s, 'count': n} for (k, a, s), n in sorted(access.items())]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--assets', type=Path, help='CVR directory, default: executable directory')
    parser.add_argument('--output', type=Path, default=Path('out/reference/scout-composition-report.json'))
    args = parser.parse_args()
    require(capstone.__version__ == '5.0.7' and unicorn.__version__ == '2.1.4', 'requires Capstone5.0.7 and Unicorn2.1.4')
    probe = Probe(args.executable)
    root = args.assets or args.executable.parent
    available = {p.name.lower(): p for p in root.iterdir() if p.is_file()}
    body, armor = [scene(available[name], digest) for name, (digest, _) in ASSETS.items()]
    synthetic = synthetic_scene()
    report = {'executable_sha256': EXE_HASH, 'capstone': capstone.__version__, 'unicorn': unicorn.__version__,
              'asset_metadata': [{k: value[k] for k in ('file', 'sha256', 'frames')} | {'objects': len(value['objects'])} for value in (body, armor)],
              'function_stubs': [], 'x87_control_word': '0x037f (explicit nearest-even, extended precision)',
              'limits': {'instruction_budget_per_case': 20000, 'timeout_microseconds_per_invocation': 2000000},
              'limitations': ['Loaded-scene pointers, geometry descriptor pairs, lookup-table handles and output storage are supplied synthetic ABI state.',
                              'Installed raw frame arrays are read independently; the original file loader and allocator are not executed.',
                              'No geometry descriptor or lookup table is dereferenced. No geometry, matrix application, axis, rasterizer or pixel fidelity is established.',
                              'Selector integration supplies explicit reactor0; no original default reactor or SMAX claim.'],
              'cases': [], 'negative_controls': []}
    for frame in (0, 1, 14, 15, 16, 17, 31, 32, 33):
        report['cases'].append(probe.run('body-direct-' + str(frame), [body], frame=frame))
        report['cases'].append(probe.run('body-armor-direct-' + str(frame), [body, armor], frame=frame))
    for frame in (0, 14, 15, 16, 29, 30):
        for armored in (False, True):
            report['cases'].append(probe.run('selector-' + str(frame) + ('-armor' if armored else '-scout'), [body, armor] if armored else [body], frame=frame, wrapper=True))
    for frame in (0, 1, 2, 3):
        report['cases'].append(probe.run('synthetic-' + str(frame), [synthetic], frame=frame, offset=(1.25, -2.5, 7.75), initial=2))
    for initial in (199, 200):
        report['cases'].append(probe.run('capacity-' + str(initial), [synthetic], initial=initial))
    for name, kwargs, expected in [('budget', {'budget': 1}, 'instruction budget'),
                                    ('code', {'deny': 'code'}, 'unexpected code'),
                                    ('read', {'deny': 'read'}, 'unexpected read'),
                                    ('write', {'deny': 'write'}, 'unexpected write'),
                                    ('omit-offset-oracle', {'oracle_mutation': 'omit-offset'}, 'frame-data mismatch'),
                                    ('transpose-matrix-oracle', {'oracle_mutation': 'transpose-matrix'}, 'frame-data mismatch')]:
        try:
            probe.run('negative-' + name, [synthetic], **kwargs)
        except RuntimeError as error:
            require(expected in str(error), 'wrong negative-control failure: ' + str(error))
            report['negative_controls'].append({'name': name, 'expected_failure': str(error)})
        else:
            raise RuntimeError('negative control did not reject: ' + name)
    report['status'] = 'passed'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': report['status'], 'cases': len(report['cases']), 'negative_controls': len(report['negative_controls']),
                      'instruction_counts': sorted(set(c['instruction_count'] for c in report['cases']))}, indent=2))


if __name__ == '__main__':
    main()
