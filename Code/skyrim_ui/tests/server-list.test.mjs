// Offline component tests: real Angular FormControl/RxJS; no HTTP, CEF or game.
import '@angular/compiler';
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { createRequire } from 'node:module';
import { mkdtempSync, rmSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { BehaviorSubject, of } from 'rxjs';
import { TestScheduler } from 'rxjs/testing';

const root = fileURLToPath(new URL('../', import.meta.url));
const require = createRequire(join(root, 'package.json'));
const buildRequire = createRequire(require.resolve('@angular-devkit/build-angular/package.json'));
const { build } = buildRequire('esbuild');
const temp = mkdtempSync(join(root, '.server-list-fixture-'));
// SoundService's imported enum initializes an otherwise unused audio holder.
// No device is opened; the tested component receives an inert sound service.
globalThis.AudioContext = class SyntheticAudioContext {};
let ServerListComponent;
try {
  await build({ entryPoints: [join(root, 'src/app/components/server-list/server-list.component.ts')],
    outfile: join(temp, 'component.mjs'), bundle: true, platform: 'node', format: 'esm',
    packages: 'external', tsconfig: join(root, 'tsconfig.json'), logLevel: 'silent' });
  ({ ServerListComponent } = await import(pathToFileURL(join(temp, 'component.mjs'))));
} finally {
  assert.equal(dirname(temp), root.replace(/[\\/]$/, ''));
  rmSync(temp, { recursive: true });
}

function fixture() {
  const server = (name, desc, options = {}) => ({ name, desc, version: 'build', ip: '127.0.0.1', port: 10578,
    player_count: 0, max_player_count: 2, pass: false, countryCode: 'fr', ...options });
  const servers = [server('Alpha', 'Snow and dragons'), server('Beta', 'Forest'),
    server('Locked', '', { pass: true }), server('Full', '', { player_count: 2 }),
    server('Old', '', { version: 'old' })];
  const ui = { getHideVersionMismatchedServers: () => true, getHideFullServers: () => true,
    getHidePasswordProtectedServers: () => true, setHideVersionMismatchedServers() {},
    setHideFullServers() {}, setHidePasswordProtectedServers() {} };
  return new ServerListComponent({}, { getServerList: () => of(servers) },
    { versionSet: new BehaviorSubject('build') }, { settings: { fontSize: of('m') } },
    {}, { get: () => '[]' }, ui);
}
const names = rows => rows.map(s => s.name).sort();

test('initial list, case-insensitive name/description, null and empty search', () => {
  new TestScheduler(assert.deepEqual).run(({ flush }) => {
    const c = fixture(), seen = [];
    const sub = c.filteredServerlist$.subscribe(rows => seen.push(names(rows)));
    assert.deepEqual(seen.at(-1), ['Alpha', 'Beta']);
    flush(); c.formSearch.setValue('ALPHA');
    assert.deepEqual(seen.at(-1), ['Alpha']);
    flush(); c.formSearch.setValue('forest');
    assert.deepEqual(seen.at(-1), ['Beta']);
    flush(); c.formSearch.reset();
    assert.equal(c.formSearch.value, null);
    assert.deepEqual(seen.at(-1), ['Alpha', 'Beta']);
    flush(); c.formSearch.setValue('');
    assert.deepEqual(seen.at(-1), ['Alpha', 'Beta']);
    sub.unsubscribe();
  });
});
test('filter changes recompute without typing and retain compatibility fields', () => {
  const c = fixture(); let rows;
  const sub = c.filteredServerlist$.subscribe(v => rows = v);
  c.onHideFullServers(false); assert.ok(rows.some(s => s.name === 'Full' && s.isFull));
  c.onHidePasswordProtectedServers(false); assert.ok(rows.some(s => s.name === 'Locked' && s.hasPassword));
  c.onHideVersionMismatchedServers(false); assert.ok(rows.some(s => s.name === 'Old' && !s.isCompatible));
  c.onHideFullServers(true); assert.ok(!rows.some(s => s.name === 'Full'));
  sub.unsubscribe();
});
test('leading 300ms throttle is retained; subscription reads current value', () => {
  new TestScheduler(assert.deepEqual).run(({ flush }) => {
    const c = fixture(); let rows;
    const sub = c.filteredServerlist$.subscribe(v => rows = v);
    c.formSearch.setValue('Alpha'); // initial emission opened the throttle window
    assert.deepEqual(names(rows), ['Alpha', 'Beta']);
    flush(); c.formSearch.setValue('Beta'); assert.deepEqual(names(rows), ['Beta']);
    sub.unsubscribe();
    c.formSearch.setValue('forest', { emitEvent: false });
    const again = c.filteredServerlist$.subscribe(v => rows = v);
    assert.deepEqual(names(rows), ['Beta']);
    again.unsubscribe();
  });
});
