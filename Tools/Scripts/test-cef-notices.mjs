
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { brotliCompressSync, gzipSync } from 'node:zlib';
import { mkdtempSync, writeFileSync, readFileSync, existsSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { readNotice, collect } from './collect-cef-notices.mjs';
function pak(entries) {
  const size = 12 + (entries.length + 1) * 6;
  const head = Buffer.alloc(size); head.writeUInt32LE(5); head.writeUInt16LE(entries.length, 8);
  let offset = size;
  entries.forEach(([id, bytes], i) => { head.writeUInt16LE(id, 12+i*6); head.writeUInt32LE(offset, 14+i*6); offset += bytes.length; });
  head.writeUInt32LE(offset, 14+entries.length*6);
  return Buffer.concat([head,...entries.map(e=>e[1])]);
}
const plain = Buffer.from('SYNTHETIC fixture Copyright; Redistribution and use.');
test('plain and gzip notices preserve exact bytes',()=>{
  assert.deepEqual(readNotice(pak([[1,plain]]),1),plain);
  assert.deepEqual(readNotice(pak([[1,gzipSync(plain)]]),1),plain);
});
test('Chromium brotli header validates length',()=>{
  const h=Buffer.alloc(8);h[0]=0x1e;h[1]=0x9b;h.writeUIntLE(plain.length,2,6);
  const compressed=Buffer.concat([h,brotliCompressSync(plain)]);
  assert.deepEqual(readNotice(pak([[1,compressed]]),1),plain);
  compressed[2]++;assert.throws(()=>readNotice(pak([[1,compressed]]),1),/length mismatch/);
});
test('missing, truncated, wrong version and bad offsets fail closed',()=>{
  assert.throws(()=>readNotice(pak([[1,plain]]),2),/Missing/);
  assert.throws(()=>readNotice(Buffer.alloc(2),1),/DataPack/);
  const b=pak([[1,plain]]);b.writeUInt32LE(99,0);assert.throws(()=>readNotice(b,1),/DataPack/);
  b.writeUInt32LE(5,0);b.writeUInt32LE(0,14);assert.throws(()=>readNotice(b,1),/bounds/);
});
test('outputs isolated under path with spaces; repeat identical; stale refused',()=>{
  const dir=mkdtempSync(join(tmpdir(),'stre notice fixture '));
  try {
    const bytes=pak([55750,38961,38960,38962].map(id=>[id,plain]));
    writeFileSync(join(dir,'resources.pak'),bytes);
    collect(dir);collect(dir);
    assert.deepEqual(readFileSync(join(dir,'resources.pak')),bytes);
    assert.deepEqual(readFileSync(join(dir,'Licenses','CEF-LICENSE.txt')),plain);
    writeFileSync(join(dir,'Licenses','CEF-LICENSE.txt'),'stale');
    assert.throws(()=>collect(dir),/Stale/);
  } finally { rmSync(dir,{recursive:true,force:true}); }
});
test('missing required notice writes no partial bundle',()=>{
  const dir=mkdtempSync(join(tmpdir(),'stre missing notice '));
  try {
    writeFileSync(join(dir,'resources.pak'),pak([[55750,plain]]));
    assert.throws(()=>collect(dir),/Missing/);assert.equal(existsSync(join(dir,'Licenses')),false);
  } finally { rmSync(dir,{recursive:true,force:true}); }
});
