
// Extract the notices already embedded in the pinned CEF runtime; never launch it.
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { brotliDecompressSync, gunzipSync } from 'node:zlib';

export function readNotice(pak, id) {
  if (pak.length < 12 || pak.readUInt32LE(0) !== 5) throw Error('Expected CEF DataPack v5');
  const count = pak.readUInt16LE(8), aliases = pak.readUInt16LE(10);
  const tableEnd = 12 + (count + 1) * 6 + aliases * 4;
  if (tableEnd > pak.length) throw Error('Truncated CEF resource table');
  let result;
  for (let i = 0; i < count; i++) {
    const pos = 12 + i * 6;
    const start = pak.readUInt32LE(pos + 2), end = pak.readUInt32LE(pos + 8);
    if (start < tableEnd || end < start || end > pak.length) throw Error('Invalid CEF resource bounds');
    if (pak.readUInt16LE(pos) === id) {
      if (result) throw Error('Duplicate CEF notice resource');
      result = pak.subarray(start, end);
    }
  }
  if (!result) throw Error(`Missing CEF notice ${id}; recheck pinned CEF resource IDs`);
  if (result.length >= 8 && result[0] === 0x1e && result[1] === 0x9b) {
    const size = result.readUIntLE(2, 6);
    if (size > 32 * 1024 * 1024) throw Error('CEF notice exceeds size limit');
    result = brotliDecompressSync(result.subarray(8), { maxOutputLength: 32 * 1024 * 1024 });
    if (result.length !== size) throw Error('CEF notice decompressed length mismatch');
  } else if (result[0] === 0x1f && result[1] === 0x8b) {
    result = gunzipSync(result, { maxOutputLength: 32 * 1024 * 1024 });
  }
  return result;
}

export function collect(runtimeRoot) {
  const pak = readFileSync(join(runtimeRoot, 'resources.pak'));
  // CEF 141.0.11 / Chromium 141.0.7390.123, cef_pack_resources.h.
  const notices = [
    ['CEF-LICENSE.txt', 55750, 'Redistribution and use'],
    ['Chromium-CREDITS.html', 38961, 'Copyright'],
    ['credits.css', 38960, ''],
    ['credits.js', 38962, ''],
  ].map(([name, id, marker]) => {
    const bytes = readNotice(pak, id);
    if (marker && !bytes.includes(Buffer.from(marker))) throw Error(`Invalid notice content: ${name}`);
    return [name, bytes];
  });
  const out = join(runtimeRoot, 'Licenses');
  // Refuse stale differing notices, and validate everything before writing.
  for (const [name, bytes] of notices) {
    const p = join(out, name);
    if (existsSync(p) && !readFileSync(p).equals(bytes)) throw Error(`Stale notice: ${name}`);
  }
  mkdirSync(out, { recursive: true });
  for (const [name, bytes] of notices) writeFileSync(join(out, name), bytes);
  return notices.map(([name, bytes]) => ({ name, bytes: bytes.length }));
}
if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  if (process.argv.length !== 3) throw Error('Usage: node collect-cef-notices.mjs <runtime-output-directory>');
  console.log(JSON.stringify(collect(resolve(process.argv[2])), null, 2));
}
