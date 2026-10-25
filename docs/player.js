/* Replays exported engine frames only. Input coordinates and algorithm states
 * come from traces.js; no search or clustering algorithm is reimplemented here. */
(() => {
  'use strict';
  const data = window.GEOVIZ_TRACES;
  const $ = id => document.getElementById(id);
  const canvas = $('stage'), ctx = canvas.getContext('2d');
  const colors = ['#236a91','#9b5e15','#75477f','#2d7042','#a94135','#6f6f24'];
  let selected = 0, cursor = 0, timer = null;
  data.forEach((r,i) => { const option = new Option(r.title, i); $('record').add(option); });
  function stop() { if(timer !== null) clearInterval(timer); timer=null; $('play').textContent='Play'; }
  function draw() {
    const r=data[selected], f=r.frames[cursor], W=1200,H=570;
    ctx.clearRect(0,0,W,H);ctx.fillStyle='#fcfcf7';ctx.fillRect(0,0,W,H);
    const all=[...r.points];
    if(r.mode===1) all.push(...r.polygon);
    if([1,6,7].includes(r.mode)) all.push(r.query);
    const xs=all.map(p=>p[0]),ys=all.map(p=>p[1]);
    const x0=Math.min(...xs),x1=Math.max(...xs),y0=Math.min(...ys),y1=Math.max(...ys);
    const scale=Math.min((W-150)/Math.max(1,x1-x0),(H-130)/Math.max(1,y1-y0));
    const xy=p=>[W/2+(p[0]-(x0+x1)/2)*scale,H/2-(p[1]-(y0+y1)/2)*scale];
    const line=(a,b,color='#24674d',width=2,dash=[])=>{ctx.beginPath();ctx.strokeStyle=color;ctx.lineWidth=width;ctx.setLineDash(dash);ctx.moveTo(...a);ctx.lineTo(...b);ctx.stroke();ctx.setLineDash([]);};
    const circle=(p,radius,color,fill=false)=>{ctx.beginPath();ctx.arc(...p,Math.max(.1,radius),0,Math.PI*2);ctx[fill?'fillStyle':'strokeStyle']=color;ctx.lineWidth=2;ctx[fill?'fill':'stroke']();};
    if(r.variant===4||r.variant===5){
      const tidal=r.variant===4, spacing=tidal?34:45;
      const wash=ctx.createLinearGradient(0,0,0,H);
      wash.addColorStop(0,tidal?'#f0f8f6':'#faf5eb');wash.addColorStop(1,'#fcfcf7');
      ctx.fillStyle=wash;ctx.fillRect(0,0,W,H);
      for(let row=-2;row<H/spacing+3;row++){
        let last=null;
        for(let x=0;x<=W;x+=15){
          const y=row*spacing+17*Math.sin(x/155+row*.19)+7*Math.sin(x/67-row*.27);
          if(last){line(last,[x,y],tidal?'rgba(108,165,171,.16)':'rgba(181,137,106,.15)',1);line([last[0],last[1]+6],[x,y+6],'rgba(255,251,235,.8)',2);}
          last=[x,y];
        }
      }
    }
    for(let x=30;x<W;x+=50)line([x,0],[x,H],'#e7ebe1',1);
    for(let y=30;y<H;y+=50)line([0,y],[W,y],'#e7ebe1',1);
    if(r.mode===1){ctx.beginPath();r.polygon.forEach((p,i)=>ctx[i?'lineTo':'moveTo'](...xy(p)));ctx.closePath();ctx.fillStyle='#e3eddf';ctx.fill();ctx.strokeStyle='#24674d';ctx.lineWidth=3;ctx.stroke();line(xy(r.query),[W-35,xy(r.query)[1]],'#8f5b14',2,[7,6]);}
    for(const key of ['regions','pruned'])for(const b of f[key]){
      const a=xy([Math.max(x0,b[0]),Math.max(y0,b[1])]),c=xy([Math.min(x1,b[2]),Math.min(y1,b[3])]);
      if(a[0]>c[0]||a[1]<c[1])continue;
      ctx.strokeStyle=key==='pruned'?'#b56253':'#ad812d';ctx.lineWidth=2;ctx.setLineDash(key==='pruned'?[7,6]:[]);ctx.strokeRect(a[0],c[1],c[0]-a[0],a[1]-c[1]);ctx.setLineDash([]);
    }
    r.edges.forEach(([a,b,w,blocked])=>line(xy(r.points[a]),xy(r.points[b]),blocked?'#ab3e30':'#94aa99',2,blocked?[7,6]:[]));
    if(r.mode===9)for(let i=0;i+1<r.points.length;i+=2)line(xy(r.points[i]),xy(r.points[i+1]),'#71897a',3);
    for(let i=1;i<f.chain.length;i++)line(xy(r.points[f.chain[i-1]]),xy(r.points[f.chain[i]]),'#24674d',4);
    if(f.radius>0&&f.focus>=0&&f.focus<r.points.length)circle(xy(r.points[f.focus]),f.radius*scale,'#ad812d');
    ctx.font='17px Lato, sans-serif';
    const boxes=[];
    r.points.forEach((p,i)=>{
      const q=xy(p);let color='#395d6f',kind=-1;
      if([4,8].includes(r.mode)&&i<f.labels.length){const label=f.labels[i];color=label>=0?colors[label%colors.length]:label===-1?'#ab3e30':'#8b9b8e';kind=label===-2?-1:f.kind[i];}
      if(f.active.includes(i))circle(q,12,'#ad812d');
      if(kind===0){line([q[0]-5,q[1]-5],[q[0]+5,q[1]+5],color,2);line([q[0]-5,q[1]+5],[q[0]+5,q[1]-5],color,2);}
      else if(kind===1){ctx.strokeStyle=color;ctx.lineWidth=2;ctx.strokeRect(q[0]-5,q[1]-5,10,10);}
      else circle(q,6,color,true);
      if(f.focus===i){ctx.strokeStyle='#ad812d';ctx.lineWidth=2;ctx.strokeRect(q[0]-14,q[1]-14,28,28);}
      const label=String(i),w=ctx.measureText(label).width+5;
      if(r.points.length<=40||f.focus===i)for(const [dx,dy]of [[10,-12],[10,22],[-w-10,-12],[-w-10,22]]){
        const b=[q[0]+dx,q[1]+dy-16,w,21];
        if(boxes.some(a=>b[0]<a[0]+a[2]&&b[0]+b[2]>a[0]&&b[1]<a[1]+a[3]&&b[1]+b[3]>a[1]))continue;
        if(r.points.some((other,j)=>{const o=xy(other);return j!==i&&o[0]>b[0]-7&&o[0]<b[0]+b[2]+7&&o[1]>b[1]-7&&o[1]<b[1]+b[3]+7;}))continue;
        boxes.push(b);ctx.fillStyle='#fcfcf7ee';ctx.fillRect(...b);ctx.fillStyle=color;ctx.fillText(label,b[0]+2,b[1]+16);break;
      }
    });
    if([1,6,7].includes(r.mode)){const q=xy(r.query);line([q[0]-10,q[1]],[q[0]+10,q[1]],'#273430');line([q[0],q[1]-10],[q[0],q[1]+10],'#273430');ctx.fillStyle='#273430';ctx.fillText('query',q[0]+13,q[1]+22);if(r.mode===7)circle(q,r.radius*scale,'#24674d');}
    $('timeline').max=r.frames.length-1;$('timeline').value=cursor;
    $('step-number').textContent=`STEP ${cursor+1} / ${r.frames.length}`;
    $('explanation').textContent=f.text;$('verification-note').textContent=r.check;
    $('back').disabled=cursor===0;$('next').disabled=cursor===r.frames.length-1;
  }
  function choose(){stop();selected=Number($('record').value);cursor=0;draw();}
  $('record').addEventListener('change',choose);
  $('timeline').addEventListener('input',()=>{stop();cursor=Number($('timeline').value);draw();});
  $('rewind').onclick=()=>{stop();cursor=0;draw();};
  $('back').onclick=()=>{stop();cursor=Math.max(0,cursor-1);draw();};
  $('next').onclick=()=>{stop();cursor=Math.min(data[selected].frames.length-1,cursor+1);draw();};
  $('final').onclick=()=>{stop();cursor=data[selected].frames.length-1;draw();};
  $('play').onclick=()=>{if(timer!==null){stop();return;}if(cursor===data[selected].frames.length-1)cursor=0;$('play').textContent='Pause';timer=setInterval(()=>{cursor++;if(cursor>=data[selected].frames.length-1){cursor=data[selected].frames.length-1;stop();}draw();},Number($('speed').value));};
  $('speed').onchange=()=>{if(timer!==null){stop();$('play').click();}};
  function page(){const hash=decodeURIComponent(location.hash.slice(1).split('?')[0])||'overview';const section=hash.split('--')[0];const valid=document.getElementById(section)?section:'overview';document.querySelectorAll('main>section').forEach(s=>s.hidden=s.id!==valid);document.querySelectorAll('nav a').forEach(a=>a.classList.toggle('active',a.dataset.page===valid));if(valid!=='overview')stop();const target=document.getElementById(hash);if(target&&hash.includes('--'))target.scrollIntoView({block:'start'});else window.scrollTo(0,0);}
  window.addEventListener('hashchange',page);page();draw();document.fonts.ready.then(draw);
  function chart(title,entries){const section=document.createElement('article'),h=document.createElement('h3');h.textContent=title;section.append(h);const max=Math.max(...entries.map(e=>e[1]),1);entries.forEach(([name,value],i)=>{const row=document.createElement('div');row.className='bar-row';const label=document.createElement('span');label.textContent=name;const track=document.createElement('div'),bar=document.createElement('div');bar.className='bar'+(i?' alt':'');bar.style.width=`${Math.max(1,value/max*100)}%`;track.append(bar);const count=document.createElement('span');count.textContent=value;row.append(label,track,count);section.append(row);});$('charts').append(section);}
  chart('Route expansions',[['Dijkstra',data[2].expanded],['A*',data[3].expanded]]);
  chart('Density distance tests',[['Direct scan',data[4].operations],['Indexed',data[8].operations]]);
})();
