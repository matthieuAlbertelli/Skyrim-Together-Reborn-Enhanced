// Offline extension of collect-cef-notices. No subprocesses, downloads or tool execution.
import { readFileSync, readdirSync, realpathSync, existsSync } from 'node:fs';
import { resolve, relative, join, isAbsolute } from 'node:path';
import { createHash } from 'node:crypto';

export const hash = bytes => createHash('sha256').update(bytes).digest('hex');
const normalizedHash = bytes => hash(Buffer.from(bytes.toString('utf8').replace(/\r\n/g, '\n')));
export function within(root, name) {
  if (typeof name !== 'string' || !name || name.includes('\\') || name.includes(':') ||
      isAbsolute(name) || name.split('/').some(p => !p || p === '..' || p === '.')) {
    throw Error(`Unsafe notice path: ${name}`);
  }
  return join(root, name);
}
function readInside(root, name) {
  const file = within(root, name);
  const rel = relative(realpathSync(root), realpathSync(file));
  if (isAbsolute(rel) || rel === '..' || rel.startsWith('..\\') || rel.startsWith('../')) throw Error(`Escaping notice path: ${name}`);
  return readFileSync(file);
}
export function loadInventory(repoRoot) {
  const manifest = JSON.parse(readFileSync(join(repoRoot, 'Tools/Packaging/redistribution.json'), 'utf8'));
  if (manifest.schemaVersion !== 1 || manifest.releaseReady !== false ||
      !Array.isArray(manifest.components) || !manifest.components.length || !manifest.ui || !manifest.uiPackages) {
    throw Error('Unsupported redistribution inventory; this collector is not release clearance');
  }
  return manifest;
}
function noticeBytes(repoRoot, notice) {
  if (!notice.source?.startsWith('https://') || !/^[a-f0-9]{64}$/.test(notice.sha256)) throw Error('Missing notice provenance/hash');
  const bytes = readInside(join(repoRoot, 'Tools/Packaging/licenses'), notice.file);
  if (bytes.length < 100 || hash(bytes) !== notice.sha256 || !/copyright|permission|redistribution/i.test(bytes.toString())) {
    throw Error(`Missing, empty or changed notice: ${notice.file}`);
  }
  return bytes;
}
function packageRoots(cachePath, depsRoot) {
  const roots = new Map();
  const add = (name, version, root) => {
    if (!/^[A-Za-z]:[\\/]/.test(root)) throw Error(`Non-local package root: ${name}`);
    if (roots.has(name) && (roots.get(name).version !== version || resolve(roots.get(name).root) !== resolve(root))) throw Error(`Ambiguous package provenance: ${name}`);
    roots.set(name, { version, root });
  };
  const cache = readFileSync(cachePath, 'utf8');
  // Read data, never evaluate xmake's Lua cache.
  for (const m of cache.matchAll(/^    (?:\["([^"\r\n]+)"\]|([\w-]+)) = \{[\s\S]*?(?=^    (?:\["|[\w-]+ =)|^\})/gm)) {
    const version = /\bversion = "([^"\r\n]+)"/.exec(m[0]);
    const dir = /installdir = \[\[([^\]\r\n]+)\]\]/.exec(m[0]);
    if (version && dir) add(m[1] || m[2], version[1], dir[1]);
  }
  let linkCount = 0;
  // Only final delivered targets; no test executables or general cache crawl.
  for (const target of ['SkyrimImmersiveLauncher', 'SkyrimTogetherServer', 'TPProcess']) {
    const targetRoot = join(depsRoot, target, 'windows/x64');
    for (const mode of readdirSync(targetRoot, { withFileTypes: true }).filter(d => d.isDirectory())) {
      for (const file of readdirSync(join(targetRoot, mode.name)).filter(n => /\.(exe|dll)\.d$/.test(n))) {
        const text = readFileSync(join(targetRoot, mode.name, file), 'utf8').replace(/\\/g, '/');
        linkCount++;
        for (const m of text.matchAll(/([A-Za-z]:\/[^\[\]\n"]*?\/\.xmake\/packages\/[a-z]\/([^/]+)\/([^/]+)\/([a-f0-9]+)\/)/g)) {
          add(m[2], m[3], m[1].replace(/\/$/, ''));
        }
      }
    }
  }
  if (linkCount < 3) throw Error('Missing delivered-target link provenance');
  return roots;
}
export function prepareNativeNotices(repoRoot, manifest, roots) {
  const files = new Map();
  const ids = new Set();
  for (const component of manifest.components) {
    if (!component.id || ids.has(component.id) || !component.version || !component.notices?.length) throw Error('Invalid/duplicate native component');
    ids.add(component.id);
    const guard = component.guard;
    let root;
    if (guard?.kind === 'source') root = repoRoot;
    else if (guard?.kind === 'xmake') {
      const pkg = roots.get(guard.package);
      if (!pkg || pkg.version !== guard.version || component.version !== guard.version) throw Error(`Wrong/missing package version: ${component.id}`);
      root = pkg.root;
      const installed = readFileSync(join(root, 'manifest.txt'), 'utf8');
      if (!installed.includes(`name = "${guard.package}"`) || !installed.includes(`version = "${guard.version}"`)) throw Error(`Package provenance mismatch: ${component.id}`);
    } else throw Error(`Unknown provenance guard: ${component.id}`);
    if (normalizedHash(readInside(root, guard.path)) !== guard.sha256) throw Error(`Source fingerprint mismatch: ${component.id}`);
    for (const notice of component.notices) {
      const bytes = noticeBytes(repoRoot, notice), dest = `Licenses/ThirdParty/${notice.file}`;
      if (files.has(dest) && !files.get(dest).equals(bytes)) throw Error(`Conflicting notice: ${dest}`);
      files.set(dest, bytes);
    }
  }
  return files;
}
export function parseUiSections(text) {
  const starts = [...text.matchAll(/^((?:@[a-z0-9_.-]+\/)?[a-z0-9_.-]+)\r?\n(\(?(?:MIT|Apache-2\.0|BSD-3-Clause|0BSD|CC-BY-4\.0)[^\r\n]*)\r?\n/gm)];
  if (!starts.length || starts[0].index !== 0) throw Error('Unknown Angular notice format');
  const seen = new Set();
  return starts.map((m, i) => {
    if (seen.has(m[1])) throw Error(`Duplicate UI notice: ${m[1]}`);
    seen.add(m[1]);
    return { name: m[1], text: text.slice(m.index, starts[i + 1]?.index ?? text.length), body: text.slice(m.index + m[0].length, starts[i + 1]?.index ?? text.length) };
  });
}
export function prepareUiNotices(repoRoot, manifest, uiRoot, original) {
  const sections = parseUiSections(original.toString('utf8'));
  const files = new Map(), rendered = [];
  for (const section of sections) {
    const expected = manifest.uiPackages[section.name];
    if (!expected) throw Error(`Unreviewed UI component: ${section.name}`);
    let packageFile = within(uiRoot, `node_modules/${section.name}/package.json`);
    if (!existsSync(packageFile)) {
      // pnpm's transitive packages (e.g. flat) need not have a top-level link.
      const store = join(uiRoot, 'node_modules/.pnpm');
      const prefix = `${section.name.replace('/', '+')}@${expected.version}`;
      const candidates = readdirSync(store).filter(n => n === prefix || n.startsWith(`${prefix}(`) || n.startsWith(`${prefix}_`));
      if (candidates.length !== 1) throw Error(`Missing/ambiguous installed UI package: ${section.name}`);
      packageFile = within(store, `${candidates[0]}/node_modules/${section.name}/package.json`);
    }
    const pkg = JSON.parse(readFileSync(packageFile, 'utf8'));
    const repository = typeof pkg.repository === 'string' ? pkg.repository : pkg.repository?.url;
    if (pkg.name !== section.name || pkg.version !== expected.version || repository !== expected.repository) throw Error(`UI version/provenance mismatch: ${section.name}`);
    if (section.body.trim().length >= 100 && /copyright/i.test(section.body)) {
      const approved = expected.sectionSha256;
      // A prior full section or a previously generated, verified replacement.
      const replacement = manifest.ui[section.name];
      const replacementText = replacement?.notices?.length && replacement.status === 'NOTICE_COMPLETE'
        ? `${section.name}\nMIT\n${replacement.notices.map(n => noticeBytes(repoRoot, n).toString('utf8')).join('\n')}\n\n` : null;
      if (normalizedHash(Buffer.from(section.text)) !== approved && section.text !== replacementText) throw Error(`Unverified UI text: ${section.name}`);
      for (const notice of replacement?.notices ?? []) files.set(`Licenses/ThirdParty/${notice.file}`, noticeBytes(repoRoot, notice));
      // Keep complete upstream sections (Font Awesome, Transloco, RxJS etc.) byte-for-byte.
      rendered.push(section.text);
      continue;
    }
    const replacement = manifest.ui[section.name];
    if (!replacement?.notices?.length || replacement.status !== 'NOTICE_COMPLETE' || replacement.version !== pkg.version) {
      throw Error(`Incomplete UI notice: ${section.name} ${pkg.version}; rightsholder text required`);
    }
    const texts = replacement.notices.map(n => {
      const b = noticeBytes(repoRoot, n); files.set(`Licenses/ThirdParty/${n.file}`, b); return b.toString('utf8');
    });
    rendered.push(`${section.name}\nMIT\n${texts.join('\n')}\n\n`);
  }
  for (const name of Object.keys(manifest.uiPackages)) {
    if (!sections.some(s => s.name === name)) throw Error(`Missing expected UI notice section: ${name}`);
  }
  files.set('UI/3rdpartylicenses.txt', Buffer.from(rendered.join('')));
  return files;
}
export function prepareFontNotices(runtimeRoot, repoRoot, manifest) {
  const files = new Map();
  if (!manifest.fonts) return files;
  if (!manifest.fonts.files?.length || !manifest.fonts.notices?.length) throw Error('Incomplete font provenance');
  for (const font of manifest.fonts.files) {
    if (!font.source?.startsWith('https://') || !/^[a-f0-9]{64}$/.test(font.sha256)) throw Error('Missing font provenance/hash');
    // Binary TTF hashes must never undergo text/newline normalization.
    if (hash(readInside(repoRoot, font.path)) !== font.sha256 ||
        hash(readInside(runtimeRoot, font.output)) !== font.sha256) throw Error(`Font source/output mismatch: ${font.path}`);
  }
  for (const notice of manifest.fonts.notices) files.set(`Licenses/ThirdParty/${notice.file}`, noticeBytes(repoRoot, notice));
  return files;
}
export function prepareVerifiedNotices(runtimeRoot, repoRoot, cachePath, depsRoot, uiRoot) {
  const manifest = loadInventory(repoRoot);
  const files = prepareNativeNotices(repoRoot, manifest, packageRoots(cachePath, depsRoot));
  const ui = prepareUiNotices(repoRoot, manifest, uiRoot, readFileSync(join(runtimeRoot, 'UI/3rdpartylicenses.txt')));
  for (const [name, bytes] of ui) files.set(name, bytes);
  for (const [name, bytes] of prepareFontNotices(runtimeRoot, repoRoot, manifest)) files.set(name, bytes);
  files.set('Licenses/redistribution.json', Buffer.from(JSON.stringify(manifest, null, 2) + '\n'));
  return files;
}
export function verifyBundle(runtimeRoot) {
  const receipt = JSON.parse(readFileSync(join(runtimeRoot, 'Licenses/notice-bundle.json'), 'utf8'));
  if (receipt.schemaVersion !== 1 || receipt.releaseReady !== false || !Array.isArray(receipt.files) || receipt.files.length < 5) throw Error('Invalid notice bundle receipt');
  const names = new Set();
  for (const file of receipt.files) {
    if (names.has(file.path)) throw Error('Duplicate bundle path');
    names.add(file.path);
    if (hash(readInside(runtimeRoot, file.path)) !== file.sha256) throw Error(`Packaged notice missing/changed: ${file.path}`);
  }
  for (const required of ['Licenses/redistribution.json','Licenses/CEF-LICENSE.txt','Licenses/Chromium-CREDITS.html','Licenses/credits.css','Licenses/credits.js','UI/3rdpartylicenses.txt']) {
    if (!names.has(required)) throw Error(`Incomplete bundle inventory: ${required}`);
  }
  const manifest = JSON.parse(readInside(runtimeRoot, 'Licenses/redistribution.json'));
  for (const item of [...manifest.components, ...Object.values(manifest.ui), ...(manifest.fonts ? [manifest.fonts] : [])]) {
    for (const notice of item.notices) {
      const path = `Licenses/ThirdParty/${notice.file}`;
      if (!names.has(path) || hash(readInside(runtimeRoot, path)) !== notice.sha256) throw Error(`Unaccounted notice: ${path}`);
    }
  }
  for (const font of manifest.fonts?.files ?? []) {
    if (hash(readInside(runtimeRoot, font.output)) !== font.sha256) throw Error(`Packaged font changed: ${font.output}`);
  }
  const sections = parseUiSections(readInside(runtimeRoot, 'UI/3rdpartylicenses.txt').toString());
  for (const name of Object.keys(manifest.uiPackages)) {
    if (!sections.some(s => s.name === name)) throw Error(`Missing packaged UI section: ${name}`);
  }
  for (const s of sections) {
    if (s.body.trim().length < 100 || !/copyright/i.test(s.body)) throw Error(`Incomplete packaged UI notice: ${s.name}`);
    const expected = manifest.uiPackages[s.name], replacement = manifest.ui[s.name];
    if (!expected) throw Error(`Unreviewed packaged UI section: ${s.name}`);
    const replacementText = replacement?.status === 'NOTICE_COMPLETE' && replacement.version === expected.version && replacement.notices?.length
      ? `${s.name}\nMIT\n${replacement.notices.map(n => readInside(runtimeRoot, `Licenses/ThirdParty/${n.file}`).toString('utf8')).join('\n')}\n\n` : null;
    if (normalizedHash(Buffer.from(s.text)) !== expected.sectionSha256 && s.text !== replacementText) throw Error(`Changed packaged UI section: ${s.name}`);
  }
  return { files: names.size, releaseReady: false };
}
