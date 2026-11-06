# Record ported addresses (docs/research/native-port-3549.json) and write them into one source file (apply_port.py).
#   python addport.py <file under src/mod> kName=0xNEW:"evidence" [...]
import json, subprocess, sys, os
J = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/docs/research/native-port-3549.json'
H = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/heads.json'
rel = sys.argv[1]
d = json.load(open(J)); heads = json.load(open(H))
one = {}
for a in sys.argv[2:]:
    name, rest = a.split('=', 1)
    new, why = rest.split(':', 1)
    old = hex(heads[name]['old']).upper().replace('0X', '0x') if name in heads else d.get(name, [None])[0]
    d[name] = [old, '0x%X' % int(new, 16), why]
    one[name] = d[name]
json.dump(d, open(J, 'w'), indent=1)
tmp = os.path.join(os.path.dirname(H), '_one.json'); json.dump(one, open(tmp, 'w'))
here = os.path.dirname(os.path.abspath(__file__))
subprocess.run([sys.executable, os.path.join(here, 'apply_port.py'), rel, tmp], check=True)
