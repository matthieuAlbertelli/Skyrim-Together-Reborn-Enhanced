// Production-bundle presentation test. Synthetic native events, no game/server.
// All page requests are fulfilled locally or aborted before network access.
// Usage: node tests/presentation-smoke.mjs <production UI> <new output> [historical missing-resource receipt]
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
import { readFileSync, existsSync, mkdirSync, writeFileSync } from 'node:fs';
import { resolve, relative, join, extname } from 'node:path';
import { fileURLToPath } from 'node:url';
const root = process.env.STRE_UI_ROOT || fileURLToPath(new URL('../', import.meta.url));
const require = createRequire(join(root, 'package.json'));
const harness = createRequire(require.resolve('@ngx-playwright/test/package.json'));
const { chromium } = harness('@playwright/test');
const ui = resolve(process.argv[2]), output = resolve(process.argv[3]);
assert.ok(!existsSync(output), 'Use a new evidence directory');
assert.ok(existsSync(join(ui, 'index.html')));
assert.ok(readFileSync(join(ui, 'styles.css'), 'utf8').includes('Roboto Condensed'));
mkdirSync(output, { recursive: true });
const browser = await chromium.launch({ channel: 'chrome', headless: true });
const results = [], errors = [], unexpectedRequests = [], historicalMissingResources = [];
const baseline = process.argv[4] ? JSON.parse(readFileSync(process.argv[4], 'utf8')) : { missingResources: [] };
const mime = { '.html':'text/html', '.js':'application/javascript', '.css':'text/css', '.json':'application/json', '.ttf':'font/ttf', '.svg':'image/svg+xml', '.png':'image/png', '.wav':'audio/wav', '.ico':'image/x-icon' };
const creation = { visible:true, phase:'raceReview', controlsLocked:false, raceConfirmed:true, raceFormId:0x13746, raceName:'Nordique — Éowyn', selectedClassId:'class.warrior', classConfirmed:false, selectedLoadoutOptions:{}, loadoutConfirmed:false, buildConfirmed:false, serverPending:false, serverAuthoritative:false, error:'' };
const bootstrap = { active:true, screen:'lobby', connected:true, busy:false, joinCode:'A7K2', canStart:true, members:[{name:'Éowyn',present:true},{name:'Compagnon du Nord',present:true}], error:'' };
const recovery = { phase:'recovery', connected:true, resumeRequired:false, disconnectIncident:false, disconnectRecovery:true, incidentKind:'remotePlayerMissing', missingMembers:1, loadedSaveValid:true, selectedToken:'', candidates:[], roster:[{ordinal:1,present:true,local:true},{ordinal:2,present:false,local:false}], error:'' };
const item={id:'synthetic-item',name:'Épée enchantée du compagnon — Dragonslayer',category:'weapons',icon:'sword',typeLabel:'Arme / Weapon',quantity:12,offered:1};
const trade={visible:true,mode:'session',phase:'negotiating',sessionId:'fixture',localPlayerId:1,remotePlayerId:2,localConfirmed:false,remoteConfirmed:false,mutable:true,error:'',inventory:[item],localOffer:[item],remoteOffer:[]};
const scenes = [
  ['creation','characterCreation','characterCreationState',creation,'.creation-shell'],
  ...['warrior','mage','thief'].map(c=>['class-'+c,'characterCreation','characterCreationState',{...creation,phase:'classSelection',selectedClassId:'class.'+c},'.class-selection-frame']),
  ['equipment','characterCreation','characterCreationState',{...creation,phase:'loadoutSelection',classConfirmed:true},'.creation-shell'],
  ['lobby','campaignBootstrap','campaignBootstrapState',bootstrap,'.campaign-shell'],
  ['trade','trade','tradeState',trade,'.trade-frame'],
  ['recovery','str','campaignResumeState',recovery,'app-campaign-resume .campaign-shell'],
];
try {
  for (const language of ['en','fr']) for (const [width,height] of [[1920,1080],[1280,720]]) {
    const context = await browser.newContext({viewport:{width,height},serviceWorkers:'block'});
    await context.route('**/*', async route => {
      const u=new URL(route.request().url());
      if(u.origin!=='https://stre-fixture.invalid') {
        // Expected optional version discovery is stubbed, never sent externally.
        if(u.hostname==='api.github.com') return route.fulfill({json:[]});
        unexpectedRequests.push(u.href); return route.abort();
      }
      const file=resolve(ui, '.'+decodeURIComponent(u.pathname==='/'?'/index.html':u.pathname));
      if(relative(ui,file).startsWith('..') || !existsSync(file)) {
        if (baseline.missingResources.includes(u.pathname)) historicalMissingResources.push(u.pathname);
        else unexpectedRequests.push(u.href);
        return route.abort();
      }
      return route.fulfill({body:readFileSync(file),contentType:mime[extname(file)]||'application/octet-stream'});
    });
    await context.addInitScript(lang=>{
      localStorage.setItem('language',lang); localStorage.setItem('audio_muted','true');
      const handlers=new Map(); globalThis.__fixtureActions=[];
      const bridge={on:(name,fn)=>handlers.set(name,[...(handlers.get(name)||[]),fn]),off:()=>{},
        emit:(name,...args)=>(handlers.get(name)||[]).forEach(fn=>fn(...args))};
      globalThis.skyrimtogether=new Proxy(bridge,{get:(obj,key)=>key in obj?obj[key]:(...args)=>globalThis.__fixtureActions.push([key,...args])});
    },language);
    const page=await context.newPage();
    page.on('pageerror',e=>errors.push(e.message));
    await page.goto('https://stre-fixture.invalid/');
    await page.waitForSelector('app-root',{state:'attached'});
    await page.evaluate(()=>{skyrimtogether.emit('init');skyrimtogether.emit('enterGame');skyrimtogether.emit('activate');skyrimtogether.emit('setVersion','fixture');});
    await page.addStyleTag({content:'body { background: #181818; }'}); // synthetic backdrop, no layout override
    for(const [scene,surface,event,state,selector] of scenes) {
      await page.evaluate(({surface,event,state})=>{
        skyrimtogether.emit('characterCreationState',JSON.stringify({visible:false}));
        skyrimtogether.emit('tradeState',JSON.stringify({visible:false}));
        skyrimtogether.emit('uiSurface',surface);
        skyrimtogether.emit(event,JSON.stringify(state));
      },{surface,event,state});
      await page.locator(selector).waitFor({state:'visible'});
      await page.evaluate(async()=>{ await document.fonts.ready; for(const style of ['normal','italic']) for(const weight of [400,700]) await document.fonts.load(`${style} ${weight} 16px "Roboto Condensed"`); });
      await page.waitForTimeout(350);
      const metrics=await page.locator(selector).evaluate(el=>{
        const rect=el.getBoundingClientRect();
        const visible=e=>{const r=e.getBoundingClientRect();return r.width>0&&r.height>0&&getComputedStyle(e).visibility!=='hidden';};
        const overflow=[...el.querySelectorAll('h1,h2,h3,button,p,blockquote,label')].filter(visible).filter(e=>e.scrollWidth>e.clientWidth+2&&getComputedStyle(e).overflowX==='visible').map(e=>({tag:e.tagName,text:e.textContent.trim().slice(0,90),client:e.clientWidth,scroll:e.scrollWidth}));
        return {rect:{x:rect.x,y:rect.y,width:rect.width,height:rect.height},font:getComputedStyle(el).fontFamily,overflow,buttons:[...el.querySelectorAll('button')].filter(visible).length,fonts:[...document.fonts].map(f=>({family:f.family,weight:f.weight,style:f.style,status:f.status}))};
      });
      assert.match(metrics.font,/Roboto Condensed/);
      // Angular can repeat critical faces inline and in styles.css.
      assert.equal(new Set(metrics.fonts.filter(f=>f.family.replaceAll('"','')==='Roboto Condensed'&&f.status==='loaded').map(f=>`${f.weight}/${f.style}`)).size,4);
      assert.ok(metrics.rect.x>=-1&&metrics.rect.y>=-1&&metrics.rect.x+metrics.rect.width<=width+1&&metrics.rect.y+metrics.rect.height<=height+1,`${scene} outside viewport`);
      const name=`${language}-${width}x${height}-${scene}`;
      await page.screenshot({path:join(output,name+'.png')});
      results.push({name,...metrics});
    }
    await context.close();
  }
} finally {
  await browser.close();
  writeFileSync(join(output,'results.json'),JSON.stringify({synthetic:true,humanAcceptance:false,results,errors,unexpectedRequests,historicalMissingResources,baseline},null,2));
}
assert.deepEqual(errors,[]); assert.deepEqual(unexpectedRequests,[]);
assert.ok(results.every(r=>r.overflow.length===0),'Review measured text overflow in results.json');
console.log(JSON.stringify({renders:results.length,fonts:'4 actual TTFs loaded',errors,unexpectedRequests,historicalMissingResources,humanAcceptance:false}));
