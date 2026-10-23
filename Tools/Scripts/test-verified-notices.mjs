import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, mkdirSync, writeFileSync, readFileSync, existsSync, rmSync, readdirSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve, relative } from 'node:path';
import { collect } from './collect-cef-notices.mjs';
import { hash, prepareNativeNotices, prepareUiNotices, verifyBundle, within } from './verified-notices.mjs';

// Deliberately synthetic: these are parser fixtures, not rightsholder attestations.
const licence = Buffer.from('SYNTHETIC TEST ONLY\r\nCopyright (c) synthetic fixture\r\nPermission is hereby granted solely for this fixture. '.repeat(3));
const header = Buffer.from('SYNTHETIC header version 1\n');
function pak() {
  const ids = [55750, 38961, 38960, 38962], b = Buffer.from('SYNTHETIC Copyright; Redistribution and use');
  const table = Buffer.alloc(12 + (ids.length + 1) * 6); table.writeUInt32LE(5); table.writeUInt16LE(ids.length, 8);
  let pos = table.length;
  ids.forEach((id, i) => { table.writeUInt16LE(id, 12 + i * 6); table.writeUInt32LE(pos, 14 + i * 6); pos += b.length; });
  table.writeUInt32LE(pos, 14 + ids.length * 6);
  return Buffer.concat([table, ...ids.map(() => b)]);
}
function put(root, path, data) { const file = join(root, path); mkdirSync(join(file, '..'), { recursive: true }); writeFileSync(file, data); }
function fixture(t) {
  const dir = mkdtempSync(join(tmpdir(), 'stre verified notice fixture '));
  // Checked absolute fixture root, never a caller-provided deletion target.
  t.after(() => { assert.equal(relative(tmpdir(), dir).startsWith('..'), false); rmSync(dir, { recursive: true, force: true }); });
  const repo = join(dir, 'repo'), runtime = join(dir, 'runtime'), ui = join(dir, 'ui'), pkg = join(dir, '.xmake/packages/f/fixture/1.0/abcdef');
  const complete = `already-complete\nMIT\n${licence}\n\n`;
  const raw = `@test/incomplete\nMIT\n\n${complete}`;
  put(runtime, 'resources.pak', pak()); put(runtime, 'UI/3rdpartylicenses.txt', raw);
  put(repo, 'Tools/Packaging/licenses/fixture.txt', licence);
  put(pkg, 'include/fixture.h', header); put(pkg, 'manifest.txt', '{ name = "fixture", version = "1.0" }');
  const notice = { file: 'fixture.txt', sha256: hash(licence), source: 'https://example.invalid/SYNTHETIC/1.0/LICENSE' };
  const manifest = { schemaVersion: 1, releaseReady: false, components: [{ id:'fixture', version:'1.0', notices:[notice], guard:{ kind:'xmake', package:'fixture', version:'1.0', path:'include/fixture.h', sha256:hash(header) } }], ui:{ '@test/incomplete':{ version:'1.0', status:'NOTICE_COMPLETE', notices:[notice] } }, uiPackages:{ '@test/incomplete':{ version:'1.0', repository:'https://example.invalid/SYNTHETIC' }, 'already-complete':{ version:'2.0', repository:'https://example.invalid/SYNTHETIC', sectionSha256:hash(Buffer.from(complete.replace(/\r\n/g,'\n'))) } } };
  for (const [name, info] of Object.entries(manifest.uiPackages)) put(ui, `node_modules/${name}/package.json`, JSON.stringify({ name, version:info.version, repository:info.repository }));
  const cache = join(dir, 'package'), deps = join(dir, 'deps');
  put(dir, 'package', `{\n    fixture = {\n        version = "1.0",\n        installdir = [[${pkg}]]\n    }\n}`);
  for (const target of ['SkyrimImmersiveLauncher','SkyrimTogetherServer','TPProcess']) put(deps, `${target}/windows/x64/release/fixture.exe.d`, '{}');
  const save = () => put(repo, 'Tools/Packaging/redistribution.json', JSON.stringify(manifest)); save();
  return {dir,repo,runtime,ui,pkg,manifest,raw,complete,save,roots:new Map([['fixture',{version:'1.0',root:pkg}]]), options:{repoRoot:repo,cachePath:cache,depsRoot:deps,uiRoot:ui}};
}
test('native notices preserve exact bytes in an isolated plan', t => {
  const f=fixture(t), files=prepareNativeNotices(f.repo,f.manifest,f.roots);
  assert.deepEqual(files.get('Licenses/ThirdParty/fixture.txt'),licence);
  assert.equal(existsSync(join(f.runtime,'Licenses')),false);
});
for (const fault of ['missing','empty','modified','wrong-version','wrong-origin','wrong-header']) test(`reject native ${fault}`, t => {
  const f=fixture(t), path=join(f.repo,'Tools/Packaging/licenses/fixture.txt');
  if(fault==='missing')rmSync(path);
  if(fault==='empty')writeFileSync(path,'');
  if(fault==='modified')writeFileSync(path,'MIT');
  if(fault==='wrong-version')f.roots.get('fixture').version='2.0';
  if(fault==='wrong-origin')put(f.pkg,'manifest.txt','{ name = "other", version = "1.0" }');
  if(fault==='wrong-header')put(f.pkg,'include/fixture.h','different source');
  assert.throws(()=>prepareNativeNotices(f.repo,f.manifest,f.roots));
});
test('UI fills verified text and preserves complete sections verbatim; repeat stable', t => {
  const f=fixture(t);const plan=prepareUiNotices(f.repo,f.manifest,f.ui,Buffer.from(f.raw));
  const out=plan.get('UI/3rdpartylicenses.txt');assert.ok(out.toString().endsWith(f.complete));assert.ok(out.includes(licence));
  assert.deepEqual(prepareUiNotices(f.repo,f.manifest,f.ui,out).get('UI/3rdpartylicenses.txt'),out);
});
for(const fault of ['unresolved','missing-section','unexpected-section','package-version','package-origin','unverified-full-text','licence-hash']) test(`reject UI ${fault}`,t=>{
  const f=fixture(t);let raw=f.raw;
  if(fault==='unresolved')f.manifest.ui['@test/incomplete'].notices=[];
  if(fault==='missing-section')raw=raw.slice(raw.indexOf('already-complete'));
  if(fault==='unexpected-section')raw=raw.replace('already-complete','unexpected');
  if(fault==='package-version')put(f.ui,'node_modules/@test/incomplete/package.json',JSON.stringify({name:'@test/incomplete',version:'2.0',repository:'https://example.invalid/SYNTHETIC'}));
  if(fault==='package-origin')put(f.ui,'node_modules/@test/incomplete/package.json',JSON.stringify({name:'@test/incomplete',version:'1.0',repository:'https://other.invalid'}));
  if(fault==='unverified-full-text')raw=raw.replace('SYNTHETIC TEST ONLY','arbitrarily changed text');
  if(fault==='licence-hash')f.manifest.ui['@test/incomplete'].notices[0].sha256='0'.repeat(64);
  assert.throws(()=>prepareUiNotices(f.repo,f.manifest,f.ui,Buffer.from(raw)));
});
test('path escape, UNC and injected drive names rejected',()=>{
  for(const name of ['../secret','C:/private','//remote/share','dir\\file','/absolute','a/./b'])assert.throws(()=>within('root',name),/Unsafe/);
});
test('complete collector and final archive verification preserve CEF bytes',t=>{
  const f=fixture(t), before=readFileSync(join(f.runtime,'resources.pak'));
  collect(f.runtime,f.options);const cef=readFileSync(join(f.runtime,'Licenses/CEF-LICENSE.txt'));
  collect(f.runtime,f.options);assert.deepEqual(readFileSync(join(f.runtime,'resources.pak')),before);
  assert.deepEqual(readFileSync(join(f.runtime,'Licenses/CEF-LICENSE.txt')),cef);
  assert.equal(verifyBundle(f.runtime).releaseReady,false);
  writeFileSync(join(f.runtime,'Licenses/ThirdParty/fixture.txt'),'changed');
  assert.throws(()=>verifyBundle(f.runtime),/missing\/changed/);
});
test('missing UI rightsholder text fails before any new notice output',t=>{
  const f=fixture(t);f.manifest.ui['@test/incomplete'].notices=[];f.save();
  assert.throws(()=>collect(f.runtime,f.options),/Incomplete UI notice/);
  assert.equal(existsSync(join(f.runtime,'Licenses')),false);
  assert.equal(readFileSync(join(f.runtime,'UI/3rdpartylicenses.txt'),'utf8'),f.raw);
});
test('stale CEF output refused without altering original UI',t=>{
  const f=fixture(t);put(f.runtime,'Licenses/CEF-LICENSE.txt','stale');
  assert.throws(()=>collect(f.runtime,f.options),/Stale/);
  assert.equal(readFileSync(join(f.runtime,'UI/3rdpartylicenses.txt'),'utf8'),f.raw);
});
test('receipt cannot omit a required notice to hide its deletion',t=>{
  const f=fixture(t);collect(f.runtime,f.options);
  const p=join(f.runtime,'Licenses/notice-bundle.json'),r=JSON.parse(readFileSync(p));
  r.files=r.files.filter(e=>e.path!=='Licenses/ThirdParty/fixture.txt');writeFileSync(p,JSON.stringify(r));
  assert.throws(()=>verifyBundle(f.runtime),/Unaccounted/);
});
test('manifest cannot label this partial remediation as release clearance',t=>{
  const f=fixture(t);f.manifest.releaseReady=true;f.save();assert.throws(()=>collect(f.runtime,f.options),/not release clearance/);
});
for (const fault of ['omitted-section','changed-section']) test(`final bundle rejects UI ${fault} even with an updated receipt`,t=>{
  const f=fixture(t);collect(f.runtime,f.options);
  const p=join(f.runtime,'Licenses/notice-bundle.json'),r=JSON.parse(readFileSync(p));
  const ui=join(f.runtime,'UI/3rdpartylicenses.txt');let text=readFileSync(ui,'utf8');
  text=fault==='omitted-section'?text.slice(0,text.indexOf('already-complete')):text.replace('SYNTHETIC TEST ONLY','not approved text');
  writeFileSync(ui,text);r.files.find(e=>e.path==='UI/3rdpartylicenses.txt').sha256=hash(Buffer.from(text));writeFileSync(p,JSON.stringify(r));
  assert.throws(()=>verifyBundle(f.runtime),/packaged UI/);
});
test('collector has no network, subprocess or application entry points',()=>{
  for(const file of ['collect-cef-notices.mjs','verified-notices.mjs']){
    const text=readFileSync(new URL(file,import.meta.url),'utf8');
    assert.doesNotMatch(text,/from ['"](?:node:)?(?:https?|child_process|net|tls)['"]|\b(?:fetch|execSync|spawnSync)\s*\(/);
  }
});

test('font provenance covers source, packaged TTF and required licence', t => {
  const f = fixture(t), bytes = Buffer.from([0, 1, 13, 10, 255, 0, 12]);
  const notice = f.manifest.components[0].notices[0];
  f.manifest.fonts = { files: [{ path: 'fonts/synthetic.ttf', output: 'UI/assets/synthetic.ttf',
    sha256: hash(bytes), source: 'https://example.invalid/SYNTHETIC/font.ttf' }], notices: [notice] };
  put(f.repo, 'fonts/synthetic.ttf', bytes); put(f.runtime, 'UI/assets/synthetic.ttf', bytes); f.save();
  collect(f.runtime, f.options); assert.equal(verifyBundle(f.runtime).releaseReady, false);
  put(f.repo, 'fonts/synthetic.ttf', 'modified source');
  assert.throws(() => collect(f.runtime, f.options), /Font source\/output mismatch/);
  put(f.repo, 'fonts/synthetic.ttf', bytes); put(f.runtime, 'UI/assets/synthetic.ttf', 'modified output');
  assert.throws(() => collect(f.runtime, f.options), /Font source\/output mismatch/);
  assert.throws(() => verifyBundle(f.runtime), /Packaged font changed/);
});
