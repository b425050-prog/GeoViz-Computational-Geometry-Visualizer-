// Optional documentation authoring helper. The generated guide has no network
// requests, framework, build step, or package dependency when opened locally.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const {marked} = process.argv[2] ? await import(pathToFileURL(path.resolve(process.argv[2])).href) : await import('marked');
const documents = [
  ['algorithms', 'Algorithms', 'ALGORITHMS.md'], ['oop', 'OOP & C++', 'OOP_CPP.md'],
  ['security', 'Security console', 'SECURITY.md'], ['guide', 'User guide', 'USER_GUIDE.md'],
  ['samples', 'Samples', 'SAMPLES.md'], ['verification', 'Verification', 'VERIFICATION.md']
];
function render(source) {
  let html = marked.parse(source, {gfm:true});
  for (const [id, , file] of documents) html = html.replaceAll(`href="${file}"`, `href="#${id}"`);
  return html.replaceAll('href="index.html"', 'href="#overview"');
}
const sections = documents.map(([id,,file]) => `<section id="${id}" class="document" hidden>${render(fs.readFileSync(path.join(root,'docs',file),'utf8'))}</section>`).join('\n');
const nav = [['overview','Recordings'], ...documents.map(([id,title])=>[id,title])].map(([id,title])=>`<a href="#${id}" data-page="${id}">${title}</a>`).join('');
const html = `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>GeoViz | Project guide</title><link rel="icon" href="../assets/geoviz.png"><link rel="stylesheet" href="guide.css"></head>
<body><header><a class="brand" href="#overview"><img src="../assets/geoviz.png" alt="">GEOVIZ</a><span>Geometry & spatial security</span><a class="source" href="../README.md">Repository overview ↗</a></header>
<nav aria-label="Documentation">${nav}</nav><main>
<section id="overview"><div class="hero"><p class="eyebrow">OBJECT-ORIENTED PROGRAMMING PROJECT</p><h1>Computational geometry.<br><em>Spatial security analysis.</em></h1><p class="lead">A C++17 application for verified algorithm playback and source-linked security analysis.</p><p class="submission">Satyam Dhal · b425050 · Computer Science and Engineering (B)<br>Faculty: Dr. Srichandan Shobhanayak<br>International Institute of Information Technology, Bhubaneswar</p><div class="tags"><span>10 algorithm modes</span><span>60 editable scenes</span><span>9 security scenarios</span><span>Offline playback</span></div></div>
<section class="player" aria-label="Recorded algorithm viewer"><div class="player-head"><div><p class="eyebrow">ENGINE RECORDINGS</p><h2>Verified algorithm recordings.</h2></div><label>Recording<select id="record" aria-label="Choose a recording"></select></label></div>
<canvas id="stage" width="1200" height="570" aria-label="Algorithm recording; the explanation below describes each step"></canvas>
<div class="explanation"><span id="step-number"></span><p id="explanation" aria-live="polite"></p></div>
<div class="controls"><button id="rewind">↶ Rewind</button><button id="back">← Step</button><button id="play" class="primary">Play</button><button id="next">Step →</button><button id="final">Final</button><label class="speed">Speed<select id="speed"><option value="700">Slow</option><option value="240" selected>Normal</option><option value="65">Fast</option></select></label></div>
<label class="timeline-label">Recording timeline<input id="timeline" type="range" min="0" value="0"></label><p id="verification-note" class="check"></p>
</section>
<div class="feature-grid"><article><span class="number">01</span><h3>Recorded execution</h3><p>Input edits invalidate old traces. Frozen snapshots preserve previous runs, parameters and answers.</p><a href="#algorithms">Read the algorithms →</a></article><article><span class="number">02</span><h3>Shared computational engine</h3><p>Geographic events use local metres. Device behaviour uses scaled fixed-window rates. Source records stay linked.</p><a href="#security">Follow the security pipeline →</a></article><article><span class="number">03</span><h3>Object-oriented design</h3><p>Encapsulated models, algorithm Strategies, interchangeable indexes and RAII keep responsibilities explicit.</p><a href="#oop">Inspect the OOP design →</a></article></div>
<section class="comparison"><p class="eyebrow">SAME INPUT / MEASURED WORK</p><h2>Algorithm operation comparison</h2><div id="charts"></div><p class="caption">Algorithm-side counts from the recorded sample. Route expansion counts compare settled vertices. DBSCAN counts compare distance tests and exclude index construction and verification; they are not timing measurements.</p></section>
<figure><img class="screenshot" src="media/security.png" alt="GeoViz security map with the permitted polygon, source observations and inspector"><figcaption>The desktop security map reuses the geometry engine and preserves source evidence.</figcaption></figure>
<section class="download"><div><h2>Application setup</h2><p>Open <code>GeoViz.exe</code> in the repository root, choose Sample, then Run and Play.</p></div><a href="#guide">Controls & build instructions →</a></section></section>
${sections}
</main><footer>GeoViz · Satyam Dhal (b425050) · International Institute of Information Technology, Bhubaneswar</footer>
<script src="media/traces.js"></script><script src="player.js"></script></body></html>`;
fs.writeFileSync(path.join(root,'docs/index.html'), html);
console.log('Rendered offline project guide.');
