"""Export source costs and exact review obligations, never effect expectations from C++."""
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]

def main():
    catalogue = json.loads((GAME / 'content/catalogue.snapshot.json').read_text(encoding='utf-8'))['recipes']
    coverage = json.loads((GAME / 'content/runtime-evidence/coverage.json').read_text(encoding='utf-8'))['decisions']
    selected = {key: catalogue[key.split(':')[1]] for key, value in coverage.items()
                if key.startswith('recipe:SH') and value['status'] == 'unbound'}
    assert len(selected) == 101
    (HERE / 'source-contracts.json').write_text(json.dumps(selected, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    lines = ['// Generated from catalogue.snapshot.json by source_index.py; source cost oracle.',
             'const std::map<std::string,Materials> printedCosts{']
    for key, row in sorted(catalogue.items()):
        if key.startswith(('SH','MA')):
            cost = ','.join(str(row['materials'].get(k, 0)) for k in ['Iron','Copper','Carbon','Glass','Circuit'])
            lines.append('    {"' + key + '",{' + cost + '}},')
    lines.append('};')
    lines.append('const std::map<std::string,Amount> printedCooldowns{')
    for key, row in sorted(catalogue.items()):
        if key.startswith(('SH','MA')):
            lines.append('    {"' + key + '",' + str(row['cooldown'] or 0) + '},')
    lines.append('};')
    (HERE / 'source-costs.inc').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print('Exported 101 exact obligations and catalogue material costs.')

if __name__ == '__main__':
    main()
