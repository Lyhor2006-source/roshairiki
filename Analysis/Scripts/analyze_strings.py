import re

with open(r'D:\RosHairiki\ROSHAIRIKI_ORIGINAL\RyomenXCracked.dylib', 'rb') as f:
    bin_data = f.read()

def get_cstr(addr):
    if 0 <= addr < len(bin_data):
        end = bin_data.find(b'\x00', addr)
        return bin_data[addr:end].decode('utf-8', errors='ignore')
    return ''

ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')

for fn in ['RenderFloatingBar', 'RenderMainMenu', 'RenderEspOverlay', 'RenderFeatureRow', 'RyomenX_Init']:
    print(f'=== STRINGS IN {fn} ===')
    raw_lines = open(rf'd:\RosHairiki\fn_{fn}.txt', encoding='utf-8', errors='ignore').readlines()
    clean_lines = [ansi_escape.sub('', l) for l in raw_lines]
    for i, line in enumerate(clean_lines):
        m = re.search(r'adrp\s+(\w+),\s+(0x[0-9a-f]+)', line)
        if m:
            reg, page = m.group(1), int(m.group(2), 16)
            next_l = clean_lines[i+1] if i+1 < len(clean_lines) else ''
            m2 = re.search(r'add\s+' + reg + r',\s+' + reg + r',\s+(0x[0-9a-f]+)', next_l)
            if m2:
                offset = int(m2.group(1), 16)
                target = page + offset
                s = get_cstr(target)
                if s and len(s) > 1 and not s.startswith('__'):
                    print(f'  add {hex(target)}: {repr(s)}')
            m3 = re.search(r'ldr\s+\w+,\s+\[\s*' + reg + r',\s+(0x[0-9a-f]+)\s*\]', next_l)
            if m3:
                offset = int(m3.group(1), 16)
                target = page + offset
                s = get_cstr(target)
                if s and len(s) > 1 and not s.startswith('__'):
                    print(f'  ldr {hex(target)}: {repr(s)}')
