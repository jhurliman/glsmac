"""Pinned selector-only experiment; no loader/compositor/rasterizer execution."""
from collections import Counter
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys

import capstone
import unicorn
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable', type=Path, help='Installed terran.exe matching the pinned GOG hash')
parser.add_argument('--rules', type=Path, help='Pinned alpha.txt (default: beside the executable)')
parser.add_argument('--output', type=Path, default=Path('out/reference/scout-selector-report.json'))
args = parser.parse_args()
EXE = args.executable
RULES = args.rules if args.rules is not None else EXE.with_name('alpha.txt')
EXE_HASH = '0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01'
RULES_HASH = '38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4'
STACK, STACK_SIZE, OUTPUT, OUTPUT_SIZE, STOP = 0x70000000, 0x10000, 0x71000000, 0x2000, 0x72000000
ENTRY = 0x4d87e0
CODE = [(0x4d8700, 0x4d87e0), (0x4d87e0, 0x4d88e0), (0x4d88e0, 0x4d8df4),
        (0x4d8e40, 0x4d9118), (0x4d9130, 0x4d959c)]
TABLES = [(0x4d8df4, 0x4d8e40), (0x4d9118, 0x4d9130), (0x4d959c, 0x4d95e8)]
COMPOSITION = {0x631a60: 3, 0x632060: 2, 0x6321c0: 3}
CALLEE_SAVED = {UC_X86_REG_EBX: 0x11111111, UC_X86_REG_ESI: 0x22222222,
                UC_X86_REG_EDI: 0x33333333, UC_X86_REG_EBP: 0x44444444}

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

def u32(value):
    return struct.pack('<I', value & 0xffffffff)

require(capstone.__version__ == '5.0.7', 'this probe requires Capstone 5.0.7')
require(unicorn.__version__ == '2.1.4', 'this probe requires Unicorn 2.1.4')
raw = EXE.read_bytes()
require(hashlib.sha256(raw).hexdigest() == EXE_HASH, 'executable hash mismatch')
rules_raw = RULES.read_bytes()
require(hashlib.sha256(rules_raw).hexdigest() == RULES_HASH, 'rules hash mismatch')
pe = struct.unpack_from('<I', raw, 0x3c)[0]
require(raw[pe:pe+4] == b'PE\0\0', 'not PE')
require(struct.unpack_from('<H', raw, pe+24)[0] == 0x10b, 'not PE32')
base = struct.unpack_from('<I', raw, pe+24+28)[0]
image_size = struct.unpack_from('<I', raw, pe+24+56)[0]
header_size = struct.unpack_from('<I', raw, pe+24+60)[0]
count = struct.unpack_from('<H', raw, pe+6)[0]
optional_size = struct.unpack_from('<H', raw, pe+20)[0]
sections = []
for index in range(count):
    pos = pe+24+optional_size+index*40
    size, rva, disk_size, disk_offset = struct.unpack_from('<IIII', raw, pos+8)
    sections.append((base+rva, size, disk_size, disk_offset))

def disk_read(address, size):
    for start, _, length, offset in sections:
        if start <= address and address+size <= start+length:
            return raw[offset+address-start:offset+address-start+size]
    raise RuntimeError(f'no disk bytes for {address:#x}+{size}')

def section_rows(name):
    lines = rules_raw.decode('cp1252').splitlines()
    start = lines.index('#'+name)+1
    result = []
    for line in lines[start:]:
        if line.startswith('#'):
            break
        line = line.split(';', 1)[0].strip()
        if line:
            result.append([part.strip() for part in line.split(',')])
    return result

weapons = section_rows('WEAPONS')
require(weapons[0][:4] == ['Hand Weapons', 'Gun', '1', '0'], 'unexpected Gun row')
require(weapons[1][:4] == ['Laser', 'Laser', '2', '0'], 'unexpected Laser row')

# Independent static filename-object binding, not an original loader invocation.
dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
dis.detail = True
instructions = list(dis.disasm(disk_read(0x45cb80, 0x45d180-0x45cb80), 0x45cb80))
bindings = {}
for index, ins in enumerate(instructions):
    if ins.mnemonic != 'call' or ins.op_str != '0x630d10' or index < 2:
        continue
    push, mov = instructions[index-2:index]
    if push.mnemonic != 'push' or mov.mnemonic != 'mov' or not mov.op_str.startswith('ecx, 0x'):
        continue
    pointer = int(push.op_str, 16)
    name = disk_read(pointer, 100).split(b'\0', 1)[0].decode('ascii')
    model = int(mov.op_str.split(', ')[1], 16)
    require(model not in bindings, 'duplicate initializer binding')
    bindings[model] = {'filename_literal': name, 'initializer_call': hex(ins.address), 'literal_pointer': hex(pointer)}

def within(address, size, ranges):
    return any(start <= address and address+size <= end for start, end in ranges)

def expected_calls(weapon, defense, reactor, frame, faction):
    result = [(0x631a60, 0x4d8982, [0x7af6c8, 0x6aa028, frame % 15])]
    if weapon == 1:
        result += [(0x631a60, 0x4d9014, [0x7af370, 0x7af698, 0]),
                   (0x632060, 0x4d8fcf, [0x7af6b8, 0]),
                   (0x6321c0, 0x4d8fde, [0x7af6a8, 0x78fa98+faction*0x1800, 0])]
    if defense == 1:
        result += [(0x631a60, 0x4d935b, [0x7af808, 0x6aa028, frame % 15])]
    if reactor == 1:
        result += [(0x631a60, 0x4d8888, [0x7af820, 0x6aa028, 0])]
    return result

def run_case(name, weapon=0, defense=0, reactor=0, frame=0, faction=1, variant=0,
             budget=10000, permit_frame=True, permit_output=True):
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(base, (image_size+4095)&~4095)
    uc.mem_write(base, raw[:header_size])
    for start, _, length, offset in sections:
        uc.mem_write(start, raw[offset:offset+length])
    uc.mem_map(STACK, STACK_SIZE)
    uc.mem_map(OUTPUT, OUTPUT_SIZE)
    uc.mem_map(STOP, 4096)
    supplied = []
    def supply(address, value, why):
        uc.mem_write(address, value)
        supplied.append({'address': hex(address), 'hex_bytes': value.hex(), 'reason': why})
    attack, mode = map(int, weapons[weapon][2:4])
    supply(0x9b6fb0+16*weapon, bytes([attack]), 'installed weapon attack')
    supply(0x9b6fb2+16*weapon, bytes([mode]), 'installed weapon mode')
    supply(0x8f3c60, u32(frame), 'explicit frame counter')
    args = [faction, 0, weapon, defense, reactor, 0, OUTPUT]
    sp = STACK+STACK_SIZE-0x100
    supply(sp, b''.join(u32(x) for x in [STOP]+args), 'return sentinel and seven explicit stack inputs')
    uc.reg_write(UC_X86_REG_ESP, sp)
    for register, value in CALLEE_SAVED.items():
        uc.reg_write(register, value)
    reads = TABLES+[(STACK, STACK+STACK_SIZE), (0x9b6fb0+16*weapon, 0x9b6fb0+16*weapon+1),
                   (0x9b6fb2+16*weapon, 0x9b6fb2+16*weapon+1)]
    if permit_frame:
        reads += [(0x8f3c60, 0x8f3c64)]
    writes = [(STACK, STACK+STACK_SIZE)]
    if permit_output:
        writes += [(OUTPUT+0x13cc, OUTPUT+0x13d0)]
    trace, access = [], Counter()
    instruction_counts, instruction_total = Counter(), 0
    finished = False
    stub = {'eax': 0 if variant == 0 else 7, 'ecx': 0xa1a1a1a1 if variant == 0 else 0xb2b2b2b2,
            'edx': 0xc3c3c3c3 if variant == 0 else 0xd4d4d4d4}
    def code_hook(machine, address, size, _):
        nonlocal instruction_total, finished
        if address == STOP:
            finished = True
            machine.emu_stop()
            return
        if address in COMPOSITION:
            argc = COMPOSITION[address]
            cur_sp = machine.reg_read(UC_X86_REG_ESP)
            require(within(cur_sp, (argc+1)*4, [(STACK, STACK+STACK_SIZE)]), 'hook stack outside range')
            words = struct.unpack('<'+'I'*(argc+1), machine.mem_read(cur_sp, (argc+1)*4))
            return_to, call_args = words[0], list(words[1:])
            require(within(return_to-5, 5, CODE), 'unexpected composition caller')
            require(disk_read(return_to-5, 1) == b'\xe8', 'not a direct call')
            require(machine.reg_read(UC_X86_REG_ECX) == OUTPUT, 'unexpected output receiver')
            require(call_args[0] in bindings, 'selected model lacks independent initializer binding')
            trace.append({'entry': hex(address), 'callsite': hex(return_to-5), 'ecx': hex(OUTPUT),
                          'arguments': [hex(v) for v in call_args], 'model_binding': bindings[call_args[0]],
                          'stubbed_registers': {k: hex(v) for k,v in stub.items()}})
            machine.reg_write(UC_X86_REG_EAX, stub['eax'])
            machine.reg_write(UC_X86_REG_ECX, stub['ecx'])
            machine.reg_write(UC_X86_REG_EDX, stub['edx'])
            machine.reg_write(UC_X86_REG_ESP, cur_sp+4+argc*4)
            machine.reg_write(UC_X86_REG_EIP, return_to)
            return
        require(within(address, size, CODE), f'unexpected executed code {address:#x}+{size}')
        instruction_total += 1
        require(instruction_total <= budget, 'instruction budget exceeded')
        for start, end in CODE:
            if start <= address < end:
                instruction_counts[hex(start)] += 1
                break
    def read_hook(machine, _, address, size, value, user):
        require(within(address, size, reads), f'unexpected read {address:#x}+{size} at {machine.reg_read(UC_X86_REG_EIP):#x}')
        access[('read', address, size)] += 1
    def write_hook(machine, _, address, size, value, user):
        require(within(address, size, writes), f'unexpected write {address:#x}+{size} at {machine.reg_read(UC_X86_REG_EIP):#x}')
        access[('write', address, size)] += 1
    uc.hook_add(UC_HOOK_CODE, code_hook)
    uc.hook_add(UC_HOOK_MEM_READ, read_hook)
    uc.hook_add(UC_HOOK_MEM_WRITE, write_hook)
    uc.emu_start(ENTRY, STOP+1, timeout=2_000_000, count=budget+10)
    require(finished, 'execution did not reach return sentinel')
    require(uc.reg_read(UC_X86_REG_ESP) == sp+4, 'wrong final stack cleanup')
    for register, value in CALLEE_SAVED.items():
        require(uc.reg_read(register) == value, 'callee-saved register changed')
    actual = [(int(call['entry'], 16), int(call['callsite'], 16), [int(v, 16) for v in call['arguments']]) for call in trace]
    require(actual == expected_calls(weapon, defense, reactor, frame, faction), f'{name}: complete trace mismatch: {actual}')
    return {'name': name, 'entry': hex(ENTRY), 'inputs': dict(faction=faction,chassis=0,weapon=weapon,defense=defense,reactor=reactor,abilities=0,frame_counter=frame),
            'supplied_bytes': supplied, 'stub_variant': variant, 'instruction_count': instruction_total,
            'instructions_by_routine': instruction_counts, 'calls': trace, 'termination': 'return sentinel',
            'allowed_reads': [[hex(a),hex(b)] for a,b in reads], 'allowed_writes': [[hex(a),hex(b)] for a,b in writes],
            'observed_memory_accesses': [dict(kind=k,address=hex(a),size=s,count=n) for (k,a,s),n in sorted(access.items())]}

report = {'executable': str(EXE), 'sha256': EXE_HASH, 'rules_sha256': RULES_HASH,
          'unicorn': unicorn.__version__, 'capstone': capstone.__version__, 'allowed_code': [[hex(a),hex(b)] for a,b in CODE],
          'initial_memory': {'pe_image_base':hex(base), 'pe_image_size':image_size,
                             'image':'Original PE headers and raw sections at declared RVAs; remaining mapped bytes zero.',
                             'stack':[hex(STACK),STACK_SIZE], 'output':[hex(OUTPUT),OUTPUT_SIZE], 'return_sentinel':hex(STOP),
                             'synthetic_regions':'Fresh zero-filled mapped pages; only per-case supplied_bytes are written before execution.'},
          'initial_registers':{'default':'Fresh Unicorn x86 CPU defaults, with EIP set to entry by emu_start.',
                               'esp':hex(STACK+STACK_SIZE-0x100),'ebx':'0x11111111','esi':'0x22222222','edi':'0x33333333','ebp':'0x44444444'},
          'instruction_budget':10000, 'unicorn_timeout_microseconds':2000000,
          'composition_entries_stubbed_before_execution': {hex(k):v for k,v in COMPOSITION.items()},
          'limitations': ['Synthetic component inputs and weapon fields; original table loader not executed.',
                          'All composition routines stubbed; no geometry, attachment math, rasterization or OS execution.',
                          'Explicit reactor 0/1 only; no default reactor or SMAX inference.'], 'cases': [], 'policy_controls': []}
for variant in (0,1):
    for reactor in (0,1):
        for frame in (0,14,15,16,29,30):
            report['cases'].append(run_case(f'scout-reactor{reactor}-frame{frame}-stub{variant}', reactor=reactor, frame=frame, variant=variant))
    report['cases'].append(run_case(f'laser-control-stub{variant}', weapon=1, variant=variant))
    report['cases'].append(run_case(f'armor-control-stub{variant}', defense=1, frame=16, variant=variant))
for name, kwargs, expected in [('budget', {'budget':1}, 'instruction budget exceeded'),
                               ('read', {'permit_frame':False}, 'unexpected read'),
                               ('write', {'permit_output':False}, 'unexpected write')]:
    try:
        run_case('policy-'+name, **kwargs)
    except RuntimeError as error:
        require(expected in str(error), f'wrong policy-control failure: {error}')
        report['policy_controls'].append({'name':name, 'expected_failure':str(error)})
    else:
        raise RuntimeError('policy control incorrectly passed: '+name)
report['status'] = 'passed'
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(json.dumps({'status':report['status'], 'cases':len(report['cases']), 'policy_controls':len(report['policy_controls']),
                  'instruction_counts':sorted(set(c['instruction_count'] for c in report['cases'])), 'sha256':EXE_HASH}, indent=2))
