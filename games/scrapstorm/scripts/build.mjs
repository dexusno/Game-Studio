import { readFile, writeFile, mkdir } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const manifest = JSON.parse(await readFile(resolve(root, 'game.json'), 'utf8'));
let html = await readFile(resolve(root, 'index.html'), 'utf8');
const inputs = ['index.html', 'src/style.css', 'src/core.js', 'src/audio.js', 'src/render.js', 'src/app.js'];
const hash = createHash('sha256');
for (const path of inputs) hash.update(path).update(await readFile(resolve(root, path)));
const sourceHash = hash.digest('hex');
const info = { version: manifest.prototype_version, hash: sourceHash.slice(0, 12), sourceSha256: sourceHash };
html = html.replace('<link rel="stylesheet" href="src/style.css">', `<style>\n${await readFile(resolve(root, 'src/style.css'), 'utf8')}\n</style>`);
for (const name of ['core', 'audio', 'render', 'app']) {
  const js = (await readFile(resolve(root, `src/${name}.js`), 'utf8')).replace(/<\/script/gi, '<\\/script');
  html = html.replace(`<script src="src/${name}.js"></script>`, `<script>\n${js}\n</script>`);
}
html = html.replace('</head>', `<script>window.SIXFOLD_BUILD=${JSON.stringify(info)};</script>\n</head>`);
if (/<script\s+src=|<link[^>]+stylesheet/i.test(html)) throw new Error('Build still has external code/style references.');
await mkdir(resolve(root, 'build'), { recursive: true });
const out = resolve(root, 'build/Sixfold-Recoil-Beta.html');
await writeFile(out, html, 'utf8');
await writeFile(resolve(root, 'build/build-info.json'), JSON.stringify({ ...info, artifact: 'Sixfold-Recoil-Beta.html', bytes: Buffer.byteLength(html), inputs }, null, 2) + '\n');
console.log(`Built ${out}\n${Buffer.byteLength(html)} bytes | ${info.version} | ${info.hash}\nSelf-contained HTML; no runtime downloads.`);
