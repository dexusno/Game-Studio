"""Copy exact unbound Mara obligations and printed material costs; no C++ oracle."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]


def main():
    catalogue = json.loads((GAME / 'content/catalogue.snapshot.json').read_text(encoding='utf-8'))['recipes']
    coverage = json.loads((GAME / 'content/runtime-evidence/coverage.json').read_text(encoding='utf-8'))['decisions']
    selected = {key: catalogue[key.split(':')[1]] for key, value in coverage.items()
                if key.startswith('recipe:MA') and value['status'] == 'unbound'}
    if len(selected) != 107:
        raise RuntimeError('The assigned source gap is no longer exactly 107; review scope before regenerating')
    (HERE / 'source-contracts.json').write_text(json.dumps(selected, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    rows = ['// Printed source costs, independently exported from the catalogue.', 'const std::map<std::string,Materials> printedCosts{']
    for key, row in sorted(catalogue.items()):
        if key.startswith(('SH', 'MA')):
            rows.append('    {"' + key + '",{' + ','.join(str(row['materials'].get(material, 0)) for material in ('Iron', 'Copper', 'Carbon', 'Glass', 'Circuit')) + '}},')
    rows.extend(['};', 'const std::set<std::string> assignedIds{'])
    rows.extend('    "' + key.split(':')[1] + '",' for key in sorted(selected))
    rows.append('};')
    (HERE / 'source-costs.inc').write_text('\n'.join(rows) + '\n', encoding='utf-8')
    counts = {}
    for row in selected.values():
        counts[row['kind']] = counts.get(row['kind'], 0) + 1
    print(json.dumps({'assigned': len(selected), 'kinds': counts}))


if __name__ == '__main__':
    main()
