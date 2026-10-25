// Local authoring QA for the generated guide. Requires Playwright and Chrome;
// these are verification tools, not desktop application dependencies.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';
const {chromium} = process.argv[2] ? await import(pathToFileURL(path.resolve(process.argv[2])).href) : await import('playwright');
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const qa=path.join(root,'build/doc-qa');fs.mkdirSync(qa,{recursive:true});
const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:process.platform==='win32'?{executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe'}:{})});
const page=await browser.newPage({viewport:{width:1440,height:1000},deviceScaleFactor:1});
const errors=[];page.on('pageerror',error=>errors.push(String(error)));
const url=pathToFileURL(path.join(root,'docs/index.html')).href;
await page.goto(url);await page.evaluate(()=>document.fonts.ready);
if(await page.locator('#record option').count()!==24)throw Error('Missing recordings');
for(let i=0;i<24;i++){
  await page.selectOption('#record',String(i));
  await page.click('#next');
  if(!(await page.locator('#step-number').textContent()).includes('STEP 2 /'))throw Error('Step failed');
  await page.click('#final');
  if(!(await page.locator('#verification-note').textContent()).startsWith('PASS'))throw Error('Unchecked recording');
  await page.click('#rewind');
  if(!(await page.locator('#step-number').textContent()).includes('STEP 1 /'))throw Error('Rewind failed');
}
await page.selectOption('#record','0');await page.selectOption('#speed','65');
await page.click('#play');await page.waitForFunction(()=>Number(document.getElementById('timeline').value)>1);
await page.click('#play');const paused=await page.locator('#timeline').inputValue();
await page.waitForTimeout(160);if(await page.locator('#timeline').inputValue()!==paused)throw Error('Pause failed');
await page.locator('#timeline').fill('12');await page.locator('#timeline').dispatchEvent('input');
if(!(await page.locator('#step-number').textContent()).includes('STEP 13 /'))throw Error('Timeline failed');
await page.click('#final');await page.evaluate(()=>window.scrollTo(0,0));await page.waitForTimeout(100);await page.screenshot({path:path.join(qa,'guide-desktop.png'),fullPage:true});
for(const tab of ['algorithms','oop','security','guide','samples','verification']){
  await page.locator(`nav a[data-page="${tab}"]`).click();
  await page.locator(`#${tab}`).waitFor({state:"visible"});
  const images=await page.locator(`#${tab} img`).evaluateAll(imgs=>imgs.map(i=>[i.src,i.complete&&i.naturalWidth>0]));
  if(images.some(i=>!i[1]))throw Error('Missing document image');
}
for(const [section,anchor] of [['samples','security-datasets'],['security','csv-schema-and-samples']]){
  await page.goto(url+'#'+section+'--'+anchor);
  if(!await page.locator('#'+section+'--'+anchor).isVisible())throw Error('Broken guide deep link: '+anchor);
}
for(const link of await page.locator('a[href^="#"]').evaluateAll(nodes=>nodes.map(n=>n.getAttribute('href')))){
  if(link!=='#'&&!await page.evaluate(h=>Boolean(document.getElementById(decodeURIComponent(h.slice(1)))),link))throw Error('Missing guide fragment '+link);
}
await page.screenshot({path:path.join(qa,'guide-verification.png'),fullPage:true});
await page.setViewportSize({width:390,height:844});await page.goto(url+'#overview');
await page.screenshot({path:path.join(qa,'guide-mobile.png'),fullPage:true});
if(await page.evaluate(()=>document.documentElement.scrollWidth>window.innerWidth+1))throw Error('Mobile horizontal overflow');
// Rasterize the original SVG diagrams for embedding in the printable report.
await page.setViewportSize({width:920,height:400});
for(const name of ['hull','geofence','routing','dbscan','closest','kdtree','collision','security-flow','classes']){
  await page.goto(pathToFileURL(path.join(root,`docs/media/${name}.svg`)).href);
  await page.screenshot({path:path.join(qa,`${name}.png`)});
}
if(errors.length)throw Error(errors.join('\n'));
await browser.close();
fs.mkdirSync(path.join(root,'docs/verification'),{recursive:true});
fs.writeFileSync(path.join(root,'docs/verification/GUIDE_CHECKS.txt'),'PASS: 24 recordings, including tidal/terrace and six additional security scenarios; step, rewind, final, timeline, play/pause; six document tabs; file-plus-anchor deep links and all internal fragments; images; 390px layout; no JavaScript errors.\n');
console.log('PASS: offline guide controls, navigation, recordings and narrow layout.');
