// tree(3) file explorer
// home screen (3d earth) -> build time screen -> file tree explorer
// tree data comes from the c++ server in src/WebServer.cpp

// AI use: Claude (Anthropic), model Claude Opus 5.5
// I used it to learn how to use three.js for the earth and d3 for drawing the tree as I knew of the functionality but not the syntax.
// I studied the examples, then wrote this file myself.
//  Prompts: 
// - "expalin the syntax of three.js and how the spinning earth works"
// - "explain how d3 enter, exit, and transition works and its syntax"  -->

import * as THREE from 'https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.min.js';

const d3 = window.d3;

// where to get each tree from the server
const DATA_FILES = {
  'B+ Tree': '/backend/bplus',
  'B+ Tree Accesses': '/backend/bplus_accesses',

  'Splay Tree': '/backend/splay',
  'Splay Tree Accesses': '/backend/splay_accesses'
};


const CHILD_PAGE = 60;  // max items shown in a folder before "+N more"

const state = {
  implementation: 'B+ Tree',
  root: null,
  path: [],
  totalNodesAccessed: 0,
  childLimit: CHILD_PAGE,
  phase: 'home'
};

// shortcut for document.getElementById
const $ = id => document.getElementById(id);

// waits a number of milliseconds (use with await)
const wait = ms => new Promise(resolve => setTimeout(resolve, ms));

// moves a toward b a little each frame (smooth camera movement)
const lerp = (a, b, t) => a + (b - a) * t;

// shows one screen and hides the rest
function showScreen(id) {
  for (const s of document.querySelectorAll('.screen')) {
    s.classList.toggle('visible', s.id === id);
  }
}

function setImplementationClass(impl) {
  document.body.classList.toggle('mode-bplus', impl === 'B+ Tree');
  document.body.classList.toggle('mode-splay', impl === 'Splay Tree');
}

// three.js setup

const canvas = $('gl');
let renderer = null;
let camera = null;
let activeScene = null;
const camLook = new THREE.Vector3();
const frameHooks = new Set();
const clock = new THREE.Clock();
const pointer = { x: 0, y: 0 };

function initGL() {
  try {
    renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: 'high-performance' });
  } catch (err) {
    console.warn('WebGL not available, skipping 3D scenes', err);
    return false;
  }
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(window.innerWidth, window.innerHeight);
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;

  camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.01, 300);

  // resize the 3d view when the window changes size
  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  // save the mouse position so the camera can follow it
  window.addEventListener('pointermove', e => {
    pointer.x = e.clientX / window.innerWidth - 0.5;
    pointer.y = e.clientY / window.innerHeight - 0.5;
  });

  // draws the scene every frame
  renderer.setAnimationLoop(() => {
    const dt = Math.min(clock.getDelta(), 0.05);
    const t = clock.elapsedTime;
    for (const hook of frameHooks) hook(dt, t);
    if (activeScene) {
      camera.lookAt(camLook);
      renderer.render(activeScene, camera);
    }
  });
  return true;
}

// home screen earth

const earthWorld = {};

function buildEarthScene() {
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x02030a);

  const loader = new THREE.TextureLoader();
  const TEXTURES = 'https://cdn.jsdelivr.net/gh/vasturiano/three-globe@1e1f4400b80179809f761e632e207d9a3cfc298e/example/img/';
  const day = loader.load(TEXTURES + 'earth-blue-marble.jpg');
  day.colorSpace = THREE.SRGBColorSpace;
  day.anisotropy = renderer.capabilities.getMaxAnisotropy();
  const bump = loader.load(TEXTURES + 'earth-topology.png');
  const water = loader.load(TEXTURES + 'earth-water.png');

  // tilt the earth like the real one
  const pivot = new THREE.Group();
  pivot.rotation.z = 0.41;
  scene.add(pivot);

  const earth = new THREE.Mesh(
    new THREE.SphereGeometry(1, 128, 128),
    new THREE.MeshPhongMaterial({
      map: day,
      bumpMap: bump,
      bumpScale: 4,
      specularMap: water,
      specular: new THREE.Color(0x4a6a8a),
      shininess: 22
    })
  );
  earth.rotation.y = -1.9;  // start facing the americas
  pivot.add(earth);

  // glow on the edge
  const rim = new THREE.Mesh(
    new THREE.SphereGeometry(1.002, 96, 96),
    new THREE.ShaderMaterial({
      uniforms: { color: { value: new THREE.Color(0x5fb4ff) } },
      vertexShader: `
        varying vec3 vN; varying vec3 vV;
        void main() {
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          vN = normalize(normalMatrix * normal);
          vV = normalize(-mv.xyz);
          gl_Position = projectionMatrix * mv;
        }`,
      fragmentShader: `
        uniform vec3 color; varying vec3 vN; varying vec3 vV;
        void main() {
          float f = pow(1.0 - max(dot(vN, vV), 0.0), 3.0);
          gl_FragColor = vec4(color, f * 0.85);
        }`,
      transparent: true,
      blending: THREE.AdditiveBlending,
      depthWrite: false
    })
  );
  pivot.add(rim);

  // atmosphere
  const atmosphere = new THREE.Mesh(
    new THREE.SphereGeometry(1.14, 96, 96),
    new THREE.ShaderMaterial({
      uniforms: { color: { value: new THREE.Color(0x3d8bff) } },
      vertexShader: `
        varying vec3 vN; varying vec3 vV;
        void main() {
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          vN = normalize(normalMatrix * normal);
          vV = normalize(-mv.xyz);
          gl_Position = projectionMatrix * mv;
        }`,
      fragmentShader: `
        uniform vec3 color; varying vec3 vN; varying vec3 vV;
        void main() {
          float d = clamp(-dot(vN, vV) / 0.48, 0.0, 1.0);
          gl_FragColor = vec4(color, pow(d, 2.6) * 0.9);
        }`,
      side: THREE.BackSide,
      transparent: true,
      blending: THREE.AdditiveBlending,
      depthWrite: false
    })
  );
  scene.add(atmosphere);

  // stars
  const starCount = 5000;
  const starPos = new Float32Array(starCount * 3);
  const starCol = new Float32Array(starCount * 3);
  for (let i = 0; i < starCount; i++) {
    const v = new THREE.Vector3().randomDirection().multiplyScalar(60 + Math.random() * 60);
    starPos.set([v.x, v.y, v.z], i * 3);
    const b = 0.35 + Math.random() * 0.65;
    const warm = Math.random();
    starCol.set([b, b * (0.9 + warm * 0.1), b * (0.85 + (1 - warm) * 0.15)], i * 3);
  }
  const starGeo = new THREE.BufferGeometry();
  starGeo.setAttribute('position', new THREE.BufferAttribute(starPos, 3));
  starGeo.setAttribute('color', new THREE.BufferAttribute(starCol, 3));
  const stars = new THREE.Points(starGeo, new THREE.PointsMaterial({
    size: 1.6, sizeAttenuation: false, vertexColors: true, transparent: true, opacity: 0.9
  }));
  scene.add(stars);

  const sun = new THREE.DirectionalLight(0xffffff, 3.2);
  sun.position.set(-2.5, 1.6, 3);
  scene.add(sun);
  scene.add(new THREE.AmbientLight(0x30406a, 0.45));

  Object.assign(earthWorld, { scene, pivot, earth, stars });

  // every frame: spin the earth and move the camera with the mouse
  frameHooks.add((dt) => {
    if (activeScene !== scene) return;
    earth.rotateY(dt * 0.045);
    stars.rotation.y += dt * 0.003;
    if (state.phase === 'home') {
      // move a little with the mouse
      camera.position.x = lerp(camera.position.x, pointer.x * 0.25, 0.04);
      camera.position.y = lerp(camera.position.y, HOME_CAM.pos.y - pointer.y * 0.12, 0.04);
    }
  });
}

// where the camera sits on the home screen
const HOME_CAM = {
  pos: new THREE.Vector3(0, -0.15, 2.75),
  look: new THREE.Vector3(0, 0.62, 0)
};

// show the earth and move the camera back to the home spot
function enterHomeView() {
  activeScene = earthWorld.scene;
  camera.position.copy(HOME_CAM.pos);
  camLook.copy(HOME_CAM.look);
}

// black screen that shows how long each tree took to build, then fades into the explorer
async function showBuildTimes(dataPromise) {
  state.phase = 'loading';
  canvas.classList.add('dim');
  showScreen('loading');

  try {
    const response = await fetch('/backend/build_times');
    const times = await response.json();
    $('bplus-time').textContent = times.bplus;
    $('splay-time').textContent = times.splay;
  } catch (error) {
    console.error(error);
  }

  await wait(2500);

  let data = null, error = null;
  try { data = await dataPromise; } catch (e) { error = e; }
  await openExplorer(data, error);
}

// loading data

async function fetchTree(impl) {
  const response = await fetch(DATA_FILES[impl]);
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  const data = await response.json();
  // give each node an id for d3
  let id = 0;
  const stack = [data.root];
  while (stack.length) {
    const n = stack.pop();
    n._id = id++;
    if (n.children) for (const c of n.children) stack.push(c);
  }
  return data;
}

// file tree explorer

let svg, gZoom, gLinks, gNodes, gFx, zoom;
let positions = new Map();  // where each node was last drawn

function initTreeView() {
  const box = $('file-display');
  box.innerHTML = '';
  svg = d3.select(box).append('svg');

  const defs = svg.append('defs');
  const grad = defs.append('radialGradient').attr('id', 'pulse-grad');
  grad.append('stop').attr('offset', '0%').attr('stop-color', '#fff');
  grad.append('stop').attr('offset', '100%').attr('stop-color', '#fff').attr('stop-opacity', 0);

  gZoom = svg.append('g');
  gLinks = gZoom.append('g');
  gNodes = gZoom.append('g');
  gFx = gZoom.append('g');
  zoom = d3.zoom()
    .scaleExtent([0.12, 3])
    .on('zoom', e => gZoom.attr('transform', e.transform));
  svg.call(zoom).on('dblclick.zoom', null);
  positions = new Map();
}

function showTreeMessage(html) {
  $('file-display').innerHTML = `<p class="empty-msg">${html}</p>`;
  svg = null;
}

async function openExplorer(data, error) {
  state.phase = 'explorer';
  showScreen('explorer');
  if (renderer) {
    canvas.classList.add('dim');
    setTimeout(() => { if (state.phase === 'explorer') activeScene = null; }, 1300);
  }

  if (error || !data) {
    showTreeMessage(
      `Couldn't reach the C++ server at <b>${DATA_FILES[state.implementation]}</b>.<br>` +
      `Start it with <b>python build.py</b> (option 3), then open <b>http://127.0.0.1:8000</b>.`
    );
    console.error(error);
    updatePanel();
    return;
  }

  state.root = data.root;
  state.path = [data.root];
  state.childLimit = CHILD_PAGE;
  state.totalNodesAccessed = data.root.accesses || 0;
  $('last-access').textContent = 'Loaded root';
  initTreeView();
  updatePanel();

  // draw root first, then its children
  renderTree({ hideChildren: true });
  await wait(400);
  renderTree();
}

function currentFolder() {
  return state.path[state.path.length - 1];
}

// what gets drawn: path from root + current folder's items
function buildView({ hideChildren = false } = {}) {
  const make = (n, kind) => ({ id: n._id, node: n, kind, children: [] });
  const top = make(state.path[0], 'path');
  let cur = top;
  for (let i = 1; i < state.path.length; i++) {
    const v = make(state.path[i], 'path');
    cur.children.push(v);
    cur = v;
  }
  if (!hideChildren) {
    const kids = currentFolder().children || [];
    for (const k of kids.slice(0, state.childLimit)) cur.children.push(make(k, 'child'));
    if (kids.length > state.childLimit) {
      cur.children.push({ id: `more-${cur.id}`, kind: 'more', count: kids.length - state.childLimit, children: [] });
    }
  }
  return d3.hierarchy(top);
}

const DY = 120;      // space between levels
const CELL_X = 104;  // grid spacing
const CELL_Y = 70;
const linkPath = d3.linkVertical().x(d => d.x).y(d => d.y);

// path goes straight down, folder items go in a grid
function layoutTree(rootH) {
  const rect = svg.node().getBoundingClientRect();
  const cols = Math.max(4, Math.min(14, Math.floor((rect.width - 80) / CELL_X)));
  let current = rootH;
  rootH.each(n => {
    if (n.data.kind === 'path') {
      n.x = 0;
      n.y = n.depth * DY;
      current = n.depth > current.depth ? n : current;
    }
  });
  const kids = current.children || [];
  kids.forEach((n, i) => {
    const row = Math.floor(i / cols);
    const inRow = Math.min(cols, kids.length - row * cols);
    n.x = (i % cols - (inRow - 1) / 2) * CELL_X;
    n.y = current.y + DY + row * CELL_Y;
    n.row = row;
  });
}

function renderTree(opts = {}) {
  if (!svg) return null;
  const rootH = buildView(opts);
  layoutTree(rootH);
  const nodes = rootH.descendants();
  const links = rootH.links();
  const current = nodes.find(n => n.data.kind === 'path' && n.data.node === currentFolder());

  const oldPos = positions;
  const newPos = new Map(nodes.map(n => [n.data.id, { x: n.x, y: n.y }]));
  const origin = oldPos.get(current.data.id) || { x: current.x, y: current.y };
  const dur = 700;
  const stagger = (d, i) => (d.data && d.data.kind !== 'path' ? Math.min(i * 9, 450) : 0);

  // lines between nodes (grow out from the parent when a folder opens)
  // AI use: Claude (Anthropic, Claude Opus 5.5) helped me learn how to make the
  // lines connect to the nodes using d3 transitions. I wrote this part myself.
  // Prompt: "how do d3 transitions make the lines connect to the nodes in my tree and grow out from parent"
  const link = gLinks.selectAll('path.link').data(links, d => d.target.data.id);

  link.exit().transition().duration(dur * 0.6)
    .attr('d', d => {
      const p = newPos.get(d.source.data.id) || origin;
      return linkPath({ source: p, target: p });
    })
    .style('opacity', 0)
    .remove();

  link.enter().append('path')
    .attr('class', 'link')
    .attr('d', d => {
      const p = oldPos.get(d.source.data.id) || newPos.get(d.source.data.id);
      return linkPath({ source: p, target: p });
    })
    .merge(link)
    .classed('on-path', d => d.target.data.kind === 'path')
    .classed('far', d => d.target.row > 0)
    .transition().duration(dur).delay((d, i) => stagger(d.target, i))
    .attr('d', d => linkPath(d));

  // nodes (grow in from the parent, shrink away when removed)
  const node = gNodes.selectAll('g.node').data(nodes, d => d.data.id);

  node.exit().transition().duration(dur * 0.6)
    .attr('transform', d => {
      const p = (d.parent && newPos.get(d.parent.data.id)) || origin;
      return `translate(${p.x},${p.y}) scale(0.01)`;
    })
    .style('opacity', 0)
    .remove();

  const entered = node.enter().append('g')
    .attr('transform', d => {
      const p = (d.parent && (oldPos.get(d.parent.data.id) || newPos.get(d.parent.data.id))) || newPos.get(d.data.id);
      return `translate(${p.x},${p.y}) scale(0.01)`;
    })
    .on('click', onNodeClick)
    .on('mouseenter', showTip)
    .on('mousemove', moveTip)
    .on('mouseleave', hideTip);

  entered.each(function (d) {
    const g = d3.select(this);
    g.append('circle').attr('class', 'hit').attr('r', 18);  // bigger click area
    if (d.data.kind === 'more' || d.data.node.folder) g.append('circle').attr('class', 'shape');
    else g.append('rect').attr('class', 'shape').attr('x', -7).attr('y', -7).attr('width', 14).attr('height', 14).attr('rx', 3);
    g.append('text');
  });

  const all = entered.merge(node);
  all.attr('class', d => {
    const k = d.data.kind;
    const isFolder = k !== 'more' && d.data.node.folder;
    return ['node', k === 'more' ? 'more' : (isFolder ? 'folder' : 'file'), k,
      k === 'path' ? 'on-path' : '', d === current ? 'current' : ''].join(' ');
  });

  all.select('circle.shape').attr('r', d => (d.data.kind === 'path' ? (d === current ? 13 : 10) : 8));

  all.select('text').each(function (d) {
    const t = d3.select(this);
    const k = d.data.kind;
    if (k === 'path') {
      t.text(truncate(d.data.node.name, 30)).attr('x', 20).attr('y', 4).attr('text-anchor', 'start');
    } else {
      const label = k === 'more' ? `+${d.data.count.toLocaleString()} more` : truncate(d.data.node.name, 14);
      t.text(label).attr('x', 0).attr('y', 25).attr('text-anchor', 'middle');
    }
  });

  all.transition().duration(dur).delay(stagger)
    .attr('transform', d => `translate(${d.x},${d.y}) scale(1)`);

  positions = newPos;
  fitView(nodes, current, dur);
  return { nodes, current, newPos };
}

function truncate(s, n) {
  return s.length > n ? s.slice(0, n - 1) + '…' : s;
}

// zoom to fit the current folder
function fitView(nodes, current, dur = 700) {
  if (!svg || !current) return;
  const rect = svg.node().getBoundingClientRect();
  const W = rect.width, H = rect.height;
  if (!W || !H) return;

  const minDepth = Math.max(0, current.depth - 2);
  const focus = nodes.filter(n => (n.data.kind === 'path' && n.depth >= minDepth) || n.parent === current);
  const xs = focus.map(n => n.x), ys = focus.map(n => n.y);
  const x0 = Math.min(...xs) - 70, x1 = Math.max(...xs) + (current.children ? 70 : 200);
  const y0 = Math.min(...ys) - 50, y1 = Math.max(...ys) + 50;
  const k = Math.max(0.3, Math.min(1.25, W / (x1 - x0), H / (y1 - y0)));
  const cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
  const transform = d3.zoomIdentity.translate(W / 2 - k * cx, H / 2 - k * cy).scale(k);
  svg.transition().duration(dur ? dur + 200 : 0).ease(d3.easeCubicInOut).call(zoom.transform, transform);
}

// clicking nodes

function onNodeClick(event, d) {
  event.stopPropagation();
  hideTip();
  const kind = d.data.kind;
  if (kind === 'more') {
    state.childLimit += CHILD_PAGE;
    renderTree();
    return;
  }
  const n = d.data.node;
  if (kind === 'path') {
    const i = state.path.indexOf(n);
    if (i < state.path.length - 1) navigateTo(i);
    return;
  }
  if (n.folder) openFolder(n);
  else searchFile(n, d);
}

function openFolder(folder) {
  state.path.push(folder);
  state.childLimit = CHILD_PAGE;
  addAccesses(folder.accesses, `Opened ${folder.name}`);
}

function navigateTo(index) {
  state.path = state.path.slice(0, index + 1);
  state.childLimit = CHILD_PAGE;
  addAccesses(currentFolder().accesses, `Back to ${currentFolder().name}`);
}

function goBack() {
  if (state.path.length > 1) navigateTo(state.path.length - 2);
}

async function searchFile(file, d) {
  // build the file's path without root (folder1/folder2/file)
  let path = "";
  for (const directory of state.path.slice(1)) {
    path += directory.name + '/';
  }
  path += file.name;

  // ask the server for the tree we're using
  let url;
  if (state.implementation === "B+ Tree") {
    url = "/backend/bplus_accesses?path=" + encodeURIComponent(path);
  } else {
    url = "/backend/splay_accesses?path=" + encodeURIComponent(path);
  }

  try {
    const response = await fetch(url);
    const accessCount = await response.json();
    file.accesses = accessCount;
  }
  catch (error) {
    console.error(error);
    return;
  }

  const from = state.totalNodesAccessed;

  state.totalNodesAccessed += file.accesses;
  animateCounter(from, state.totalNodesAccessed);
  $('last-access').textContent = `Found ${file.name} · ${file.accesses} nodes`;
  gNodes.selectAll('g.node').classed('found', x => x === d);
  const pts = state.path.map(p => positions.get(p._id)).concat([positions.get(file._id)]);
  runPulse(pts);
  toast(`Found <b>${file.name}</b> in ${file.accesses} node access${file.accesses === 1 ? '' : 'es'}`);
}

function addAccesses(n, message) {
  const from = state.totalNodesAccessed;
  state.totalNodesAccessed += n || 0;
  $('last-access').textContent = `${message} · ${n} nodes`;
  updatePanel(false);
  const result = renderTree();
  animateCounter(from, state.totalNodesAccessed);
  if (result) {
    const pts = state.path.map(p => result.newPos.get(p._id));
    setTimeout(() => runPulse(pts), 450);
  }
}

function resetCounter() {
  state.totalNodesAccessed = 0;
  $('node-access-count').textContent = '0';
  $('last-access').textContent = 'Counter reset';
}

// effects

// dot that moves down the path to show the search
function runPulse(points) {
  points = points.filter(Boolean);
  if (!gFx || points.length < 1) return;
  const dot = gFx.append('circle').attr('class', 'pulse').attr('r', 6)
    .attr('cx', points[0].x).attr('cy', points[0].y);
  let t = dot.transition().duration(0);
  for (let i = 1; i < points.length; i++) {
    t = t.transition().duration(170).ease(d3.easeQuadInOut)
      .attr('cx', points[i].x).attr('cy', points[i].y);
  }
  t.transition().duration(400).attr('r', 22).style('opacity', 0).remove();
}

function animateCounter(from, to) {
  const el = $('node-access-count');
  const hero = el.closest('.metric-hero');
  // restart the bump effect on the counter
  hero.classList.remove('bump');
  void hero.offsetWidth;
  hero.classList.add('bump');
  d3.select(el).transition().duration(600).tween('text', () => {
    const i = d3.interpolateNumber(from, to);
    return t => { el.textContent = Math.round(i(t)).toLocaleString(); };
  });
}

let toastTimer = null;
function toast(html) {
  const el = $('toast');
  el.innerHTML = html;
  el.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove('show'), 2600);
}

// tooltip

function showTip(event, d) {
  const tip = $('tooltip');
  if (d.data.kind === 'more') {
    tip.innerHTML = `<b>${d.data.count.toLocaleString()}</b> more items<br>Click to show more`;
  } else {
    const n = d.data.node;
    const kids = n.children || [];
    const folders = kids.filter(c => c.folder).length;
    tip.innerHTML = `<b>${n.name}</b><br>` + (n.folder ? `Folder · ${n.accesses} nodes to reach` : 'File · click to search') +
      (n.folder ? `<br>${folders.toLocaleString()} folders · ${(kids.length - folders).toLocaleString()} files` : '');
  }
  tip.classList.add('show');
  moveTip(event);
}

function moveTip(event) {
  const tip = $('tooltip');
  tip.style.left = `${event.clientX}px`;
  tip.style.top = `${event.clientY}px`;
}

function hideTip() {
  $('tooltip').classList.remove('show');
}

// side panel

function updatePanel(updateCounter = true) {
  const folder = currentFolder();
  const children = (folder && folder.children) || [];
  const other = state.implementation === 'B+ Tree' ? 'Splay Tree' : 'B+ Tree';

  $('current-impl').textContent = state.implementation;
  $('btn-switch').textContent = `Switch to ${other}`;
  $('current-dir').textContent = folder ? '/' + state.path.map(f => f.name).join('/') : '—';
  $('subfolder-count').textContent = children.filter(c => c.folder).length.toLocaleString();
  $('file-count').textContent = children.filter(c => !c.folder).length.toLocaleString();
  if (updateCounter) $('node-access-count').textContent = state.totalNodesAccessed.toLocaleString();
  $('btn-back').disabled = state.path.length <= 1;
}

// buttons from index.html

function selectImplementation(implName) {
  if (state.phase !== 'home') return;
  state.implementation = implName;
  setImplementationClass(implName);
  // start loading the tree while the build times show
  const dataPromise = fetchTree(implName);
  dataPromise.catch(() => {});  // any error is handled in showBuildTimes
  showBuildTimes(dataPromise);
}

async function switchImplementation() {
  const other = state.implementation === 'B+ Tree' ? 'Splay Tree' : 'B+ Tree';
  state.implementation = other;
  setImplementationClass(other);
  $('current-impl').textContent = other;
  showTreeMessage('Loading…');
  try {
    const data = await fetchTree(other);
    await openExplorer(data, null);
  } catch (e) {
    await openExplorer(null, e);
  }
}

function goHome() {
  state.phase = 'home';
  showScreen('home');
  if (renderer) {
    canvas.classList.remove('dim');
    enterHomeView();
  }
}

// makes these functions usable from the buttons in index.html
window.selectImplementation = selectImplementation;
window.switchImplementation = switchImplementation;
window.goHome = goHome;
window.goBack = goBack;
window.resetCounter = resetCounter;

// backspace goes back one folder

document.addEventListener('keydown', e => {
  if (e.key === 'Backspace' && state.phase === 'explorer' && e.target === document.body) goBack();
});

// redraw the tree when the window is resized with some delay
let resizeTimer = null;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    if (state.phase === 'explorer' && svg) renderTree();
  }, 200);
});

// start

if (initGL()) {
  buildEarthScene();
  enterHomeView();
} else {
  canvas.style.display = 'none';
  document.body.style.background =
    'radial-gradient(ellipse at 50% 120%, #123a6b 0%, #03050b 60%)';
}
