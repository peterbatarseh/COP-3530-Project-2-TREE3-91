/*
TREE(3) File System Explorer

Three scenes:
  1. HOME       3D Earth (Three.js) with the title and the two implementation choices
  2. CINEMATIC  zoom into a forest on Earth, a real 3D tree grows, then flips upside
                down (real trees grow up, data trees grow down)
  3. EXPLORER   the file tree drawn as a data structure (D3), loaded from the C++ server

Data comes from the C++ backend (src/WebServer.cpp):
  GET /backend/bplus  and  GET /backend/splay
Each returns { implementation, root: { name, folder, accesses, children: [...] } }

Three.js, D3, the Earth textures and the fonts load from the internet (jsDelivr / Google Fonts).
*/

import * as THREE from 'https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.min.js';

const d3 = window.d3;

const DATA_FILES = {
  'B+ Tree': '/backend/bplus',
  'B+ Tree Accesses': '/backend/bplus_accesses', // access a specific node by its path

  'Splay Tree': '/backend/splay',
  'Splay Tree Accesses': '/backend/splay_accesses' // access a specific node by its name
};

// Real forests the camera can dive into
const FORESTS = [
  { name: 'Sequoia National Park, USA', lat: 36.49, lon: -118.57 },
  { name: 'Amazon Rainforest, Brazil', lat: -3.47, lon: -62.37 },
  { name: 'Black Forest, Germany', lat: 48.30, lon: 8.15 },
  { name: 'Daintree Rainforest, Australia', lat: -16.17, lon: 145.42 },
  { name: 'Yakushima Forest, Japan', lat: 30.36, lon: 130.53 },
  { name: 'Bialowieza Forest, Poland', lat: 52.73, lon: 23.85 },
  { name: 'Congo Rainforest, DR Congo', lat: -0.50, lon: 22.50 },
  { name: 'Great Bear Rainforest, Canada', lat: 52.50, lon: -127.80 },
  { name: 'Taiga Forest, Siberia', lat: 60.00, lon: 100.00 },
  { name: 'Monteverde Cloud Forest, Costa Rica', lat: 10.30, lon: -84.80 }
];

const CHILD_PAGE = 60;  // how many children to draw before showing "+N more"

const state = {
  implementation: 'B+ Tree',
  root: null,
  path: [],
  totalNodesAccessed: 0,
  childLimit: CHILD_PAGE,
  phase: 'home'
};

const $ = id => document.getElementById(id);
const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

/* ============================================================
   Small animation helpers
   ============================================================ */

let skipping = false;  // set by "Skip intro" / Esc; makes every tween finish instantly

const ease = {
  inOut: t => (t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2),
  in: t => t * t * t,
  out: t => 1 - Math.pow(1 - t, 3),
  linear: t => t
};

function tween(ms, fn, easing = ease.inOut) {
  return new Promise(resolve => {
    if (skipping || ms <= 0) { fn(1); resolve(); return; }
    const start = performance.now();
    const step = now => {
      if (skipping) { fn(1); resolve(); return; }
      const t = Math.min(1, (now - start) / ms);
      fn(easing(t));
      if (t < 1) requestAnimationFrame(step); else resolve();
    };
    requestAnimationFrame(step);
  });
}

const wait = ms => tween(ms, () => {}, ease.linear);
const lerp = (a, b, t) => a + (b - a) * t;

async function typeText(el, text, msPerChar = 28) {
  if (skipping) { el.textContent = text; return; }
  el.textContent = '';
  for (let i = 1; i <= text.length; i++) {
    if (skipping) break;
    el.textContent = text.slice(0, i);
    await new Promise(r => setTimeout(r, msPerChar));
  }
  el.textContent = text;
}

function showScreen(id) {
  for (const s of document.querySelectorAll('.screen')) {
    s.classList.toggle('visible', s.id === id);
  }
}

function setImplementationClass(impl) {
  document.body.classList.toggle('mode-bplus', impl === 'B+ Tree');
  document.body.classList.toggle('mode-splay', impl === 'Splay Tree');
}

function accentColor() {
  return state.implementation === 'Splay Tree' ? 0xff6fd8 : 0x4fd8ff;
}

/* ============================================================
   Three.js setup (shared renderer + camera)
   ============================================================ */

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
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;

  camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.01, 300);

  window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  window.addEventListener('pointermove', e => {
    pointer.x = e.clientX / window.innerWidth - 0.5;
    pointer.y = e.clientY / window.innerHeight - 0.5;
  });

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

/* ============================================================
   SCENE 1 — EARTH
   ============================================================ */

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

  // Pivot holds the axial tilt; the Earth spins inside it
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
  earth.rotation.y = -1.9;  // start with the Americas facing us
  pivot.add(earth);

  // Thin rim of light on the planet's edge
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

  // Outer atmosphere glow (fades out away from the surface)
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

  // Stars
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

  Object.assign(earthWorld, { scene, pivot, earth, stars, spinning: true });

  frameHooks.add((dt) => {
    if (activeScene !== scene) return;
    if (earthWorld.spinning) earth.rotateY(dt * 0.045);
    stars.rotation.y += dt * 0.003;
    if (state.phase === 'home') {
      // gentle parallax with the mouse
      camera.position.x = lerp(camera.position.x, pointer.x * 0.25, 0.04);
      camera.position.y = lerp(camera.position.y, HOME_CAM.pos.y - pointer.y * 0.12, 0.04);
    }
  });
}

// Earth sits low in the frame like a horizon, under the title
const HOME_CAM = {
  pos: new THREE.Vector3(0, -0.15, 2.75),
  look: new THREE.Vector3(0, 0.62, 0),
  fov: 45
};

function enterHomeView() {
  activeScene = earthWorld.scene;
  earthWorld.spinning = true;
  earthWorld.pivot.rotation.z = 0.41;
  camera.fov = HOME_CAM.fov;
  camera.near = 0.01;
  camera.updateProjectionMatrix();
  camera.position.copy(HOME_CAM.pos);
  camLook.copy(HOME_CAM.look);
}

// Rotation that turns (lat, lon) on the texture to face the camera (+Z)
function quaternionFacing(lat, lon) {
  const u = (lon + 180) / 360 * Math.PI * 2;
  const theta = (90 - lat) * Math.PI / 180;
  const p = new THREE.Vector3(-Math.cos(u) * Math.sin(theta), Math.cos(theta), Math.sin(u) * Math.sin(theta));
  const yaw = -Math.atan2(p.x, p.z);
  const pitch = lat * Math.PI / 180;
  return new THREE.Quaternion().setFromEuler(new THREE.Euler(pitch, yaw, 0, 'XYZ'));
}

/* ============================================================
   SCENE 2 — FOREST + REAL 3D TREE
   ============================================================ */

const forest = {};

function mulberry32(seed) {
  return function () {
    seed |= 0; seed = seed + 0x6D2B79F5 | 0;
    let t = Math.imul(seed ^ seed >>> 15, 1 | seed);
    t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
    return ((t ^ t >>> 14) >>> 0) / 4294967296;
  };
}

function glowTexture() {
  const c = document.createElement('canvas');
  c.width = c.height = 64;
  const g = c.getContext('2d');
  const grad = g.createRadialGradient(32, 32, 0, 32, 32, 32);
  grad.addColorStop(0, 'rgba(255,255,255,1)');
  grad.addColorStop(0.25, 'rgba(255,255,255,0.6)');
  grad.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grad;
  g.fillRect(0, 0, 64, 64);
  return new THREE.CanvasTexture(c);
}

function buildRealTree(rng) {
  const bark = new THREE.MeshStandardMaterial({ color: 0x4a3122, roughness: 0.95, emissive: 0x000000 });
  const root = new THREE.Group();
  const branches = [];
  const tips = [];
  const MAX_DEPTH = 5;

  function branch(parent, len, rad, depth) {
    const g = new THREE.Group();
    parent.add(g);
    const geo = new THREE.CylinderGeometry(rad * 0.66, rad, len, 8, 1);
    geo.translate(0, len / 2, 0);
    const mesh = new THREE.Mesh(geo, bark);
    mesh.castShadow = true;
    g.add(mesh);
    branches.push({ g, depth });

    if (depth === MAX_DEPTH) {
      tips.push({ g, len });
      return g;
    }

    const count = depth === 0 ? 3 : (rng() < 0.55 ? 2 : 3);
    const base = rng() * Math.PI * 2;
    for (let i = 0; i < count; i++) {
      const child = branch(g, len * (0.68 + rng() * 0.12), rad * 0.64, depth + 1);
      child.position.y = len * (0.88 + rng() * 0.1);
      child.rotation.order = 'YXZ';
      const leader = i === 0 && depth < 3;
      const tilt = leader ? 0.12 + rng() * 0.15 : 0.5 + rng() * 0.38;
      child.rotation.set(tilt, base + i * (Math.PI * 2 / count) + (rng() - 0.5) * 0.7, 0);
    }
    return g;
  }

  branch(root, 2.3, 0.26, 0);

  // Leaves: one instanced mesh, placed around every branch tip
  root.updateMatrixWorld(true);
  const leafGeo = new THREE.IcosahedronGeometry(0.15, 0);
  const leafMat = new THREE.MeshStandardMaterial({ color: 0xffffff, roughness: 0.75, flatShading: true, emissive: 0x000000 });
  const perTip = 7;
  const leaves = new THREE.InstancedMesh(leafGeo, leafMat, tips.length * perTip);
  leaves.castShadow = true;
  const leafData = [];
  const color = new THREE.Color();
  let k = 0;
  for (const tip of tips) {
    const end = tip.g.localToWorld(new THREE.Vector3(0, tip.len, 0));
    for (let j = 0; j < perTip; j++) {
      const pos = end.clone().add(new THREE.Vector3().randomDirection().multiplyScalar(0.15 + rng() * 0.45));
      const rot = new THREE.Quaternion().setFromEuler(new THREE.Euler(rng() * 6, rng() * 6, rng() * 6));
      const size = 0.6 + rng() * 0.7;
      leafData.push({ pos, rot, size, delay: 0.72 + rng() * 0.18 });
      color.setHSL(0.24 + rng() * 0.1, 0.55 + rng() * 0.3, 0.28 + rng() * 0.2);
      leaves.setColorAt(k++, color);
    }
  }
  root.add(leaves);

  const m = new THREE.Matrix4();
  const s = new THREE.Vector3();

  function setGrowth(t) {
    const span = 1 / (MAX_DEPTH + 1);
    for (const b of branches) {
      const local = THREE.MathUtils.clamp((t - b.depth * span * 0.85) / (span * 1.7), 0, 1);
      b.g.scale.setScalar(Math.max(0.0001, ease.out(local)));
    }
    leafData.forEach((L, i) => {
      const local = THREE.MathUtils.clamp((t - L.delay) / 0.14, 0, 1);
      s.setScalar(Math.max(0.0001, ease.out(local) * L.size));
      m.compose(L.pos, L.rot, s);
      leaves.setMatrixAt(i, m);
    });
    leaves.instanceMatrix.needsUpdate = true;
  }

  setGrowth(0);
  return { root, setGrowth, bark, leafMat };
}

function buildForestScene() {
  const scene = new THREE.Scene();
  const rng = mulberry32(Math.floor(Math.random() * 1e9));

  // Dusk sky
  const sky = new THREE.Mesh(
    new THREE.SphereGeometry(120, 32, 16),
    new THREE.ShaderMaterial({
      uniforms: {
        top: { value: new THREE.Color(0x070a24) },
        horizon: { value: new THREE.Color(0xd9876a) },
        bottom: { value: new THREE.Color(0x07090c) }
      },
      vertexShader: `varying vec3 vW; void main(){ vW = position; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }`,
      fragmentShader: `
        uniform vec3 top; uniform vec3 horizon; uniform vec3 bottom; varying vec3 vW;
        void main(){
          float h = normalize(vW).y;
          vec3 c = h > 0.0 ? mix(horizon, top, pow(h, 0.45)) : mix(horizon, bottom, pow(-h, 0.3));
          gl_FragColor = vec4(c, 1.0);
        }`,
      side: THREE.BackSide,
      depthWrite: false
    })
  );
  scene.add(sky);
  scene.fog = new THREE.FogExp2(0x2a1f3d, 0.028);

  // Ground
  const ground = new THREE.Mesh(
    new THREE.CircleGeometry(60, 64),
    new THREE.MeshStandardMaterial({ color: 0x14301d, roughness: 1 })
  );
  ground.rotation.x = -Math.PI / 2;
  ground.receiveShadow = true;
  scene.add(ground);

  // A ring of dark pines in the distance
  const pineCount = 90;
  const pines = new THREE.InstancedMesh(
    new THREE.ConeGeometry(1, 1, 7).translate(0, 0.5, 0),
    new THREE.MeshStandardMaterial({ color: 0x0c1d15, roughness: 1, flatShading: true }),
    pineCount
  );
  const m = new THREE.Matrix4();
  for (let i = 0; i < pineCount; i++) {
    const a = rng() * Math.PI * 2;
    const r = 13 + rng() * 26;
    const h = 3 + rng() * 6;
    m.compose(
      new THREE.Vector3(Math.cos(a) * r, 0, Math.sin(a) * r),
      new THREE.Quaternion(),
      new THREE.Vector3(h * 0.28, h, h * 0.28)
    );
    pines.setMatrixAt(i, m);
  }
  scene.add(pines);

  // The hero tree, inside a pivot so it can flip upside down later
  const tree = buildRealTree(rng);
  const PIVOT_Y = 3.4;
  const pivot = new THREE.Group();
  pivot.position.y = PIVOT_Y;
  tree.root.position.y = -PIVOT_Y;
  pivot.add(tree.root);
  scene.add(pivot);

  // Fireflies
  const flyCount = 70;
  const flyBase = [];
  const flyPos = new Float32Array(flyCount * 3);
  for (let i = 0; i < flyCount; i++) {
    const a = rng() * Math.PI * 2;
    const r = 1.5 + rng() * 7;
    flyBase.push({ x: Math.cos(a) * r, y: 0.4 + rng() * 6, z: Math.sin(a) * r, p: rng() * 10 });
  }
  const flyGeo = new THREE.BufferGeometry();
  flyGeo.setAttribute('position', new THREE.BufferAttribute(flyPos, 3));
  const flyMat = new THREE.PointsMaterial({
    size: 0.14, map: glowTexture(), color: 0xc8ff7a, transparent: true,
    blending: THREE.AdditiveBlending, depthWrite: false
  });
  const flies = new THREE.Points(flyGeo, flyMat);
  scene.add(flies);

  // Lights
  scene.add(new THREE.HemisphereLight(0xb9a8ff, 0x1a2a1a, 1.1));
  const sun = new THREE.DirectionalLight(0xffb27a, 3.0);
  sun.position.set(-7, 9, 5);
  sun.castShadow = true;
  sun.shadow.mapSize.set(1024, 1024);
  Object.assign(sun.shadow.camera, { left: -9, right: 9, top: 12, bottom: -2, near: 1, far: 40 });
  scene.add(sun);
  const rimLight = new THREE.DirectionalLight(0x6f8cff, 1.4);
  rimLight.position.set(6, 4, -8);
  scene.add(rimLight);

  frameHooks.add((dt, t) => {
    if (activeScene !== scene) return;
    for (let i = 0; i < flyCount; i++) {
      const b = flyBase[i];
      flyPos[i * 3] = b.x + Math.sin(t * 0.6 + b.p) * 0.5;
      flyPos[i * 3 + 1] = b.y + Math.sin(t * 0.9 + b.p * 2) * 0.35;
      flyPos[i * 3 + 2] = b.z + Math.cos(t * 0.5 + b.p) * 0.5;
    }
    flyGeo.attributes.position.needsUpdate = true;
    flyMat.opacity = 0.65 + Math.sin(t * 3) * 0.25;
    tree.root.rotation.z = Math.sin(t * 0.8) * 0.008;  // breeze
  });

  Object.assign(forest, { scene, tree, pivot, ground, pines, flyMat, PIVOT_Y });
}

/* ============================================================
   The cinematic: Earth -> forest -> flip into a data tree
   ============================================================ */

async function runCinematic(dataPromise) {
  state.phase = 'cinematic';
  skipping = reducedMotion;
  showScreen('cinematic');
  const l1 = $('hud-line-1'), l2 = $('hud-line-2'), l3 = $('hud-line-3');
  l1.textContent = l2.textContent = l3.textContent = '';

  const spot = FORESTS[Math.floor(Math.random() * FORESTS.length)];
  const coords = `${Math.abs(spot.lat).toFixed(2)}°${spot.lat >= 0 ? 'N' : 'S'}  ` +
                 `${Math.abs(spot.lon).toFixed(2)}°${spot.lon >= 0 ? 'E' : 'W'}`;

  // 1. Turn the planet so the forest faces us, center the camera
  earthWorld.spinning = false;
  const earth = earthWorld.earth;
  const q0 = earth.quaternion.clone();
  const q1 = quaternionFacing(spot.lat, spot.lon);
  const tilt0 = earthWorld.pivot.rotation.z;
  const p0 = camera.position.clone();
  const look0 = camLook.clone();
  const p1 = new THREE.Vector3(0, 0, 2.9);
  const look1 = new THREE.Vector3(0, 0, 0);

  typeText(l1, `Loading ${state.implementation} · locating root node`);
  await tween(2300, t => {
    earth.quaternion.slerpQuaternions(q0, q1, t);
    earthWorld.pivot.rotation.z = lerp(tilt0, 0, t);
    camera.position.lerpVectors(p0, p1, t);
    camLook.lerpVectors(look0, look1, t);
  });

  await typeText(l2, spot.name, 32);
  typeText(l3, coords);
  await wait(500);

  // 2. Dive toward the surface
  typeText(l1, 'Descending');
  const fov0 = camera.fov;
  await tween(2100, t => {
    camera.position.z = lerp(2.9, 1.012, t);
    camera.fov = lerp(fov0, 24, t);
    camera.updateProjectionMatrix();
  }, ease.in);

  // 3. White flash, swap to the forest
  const flash = $('flash');
  await tween(260, t => { flash.style.opacity = t; }, ease.linear);
  if (!forest.scene) buildForestScene();
  forest.tree.setGrowth(0);
  forest.pivot.rotation.z = 0;
  forest.tree.leafMat.emissive.setHex(0x000000);
  forest.tree.bark.emissive.setHex(0x000000);
  activeScene = forest.scene;
  camera.fov = 50;
  camera.updateProjectionMatrix();
  camera.position.set(9, 1.6, 6);
  camLook.set(0, 3.0, 0);
  l1.textContent = l2.textContent = l3.textContent = '';
  await tween(700, t => { flash.style.opacity = 1 - t; }, ease.out);

  // 4. The real tree grows, camera orbits
  typeText(l1, 'Root node planted');
  typeText(l2, 'Real trees grow up…', 34);
  const a0 = Math.atan2(6, 9);
  await tween(4800, t => {
    forest.tree.setGrowth(t);
    const a = a0 + t * 1.2;
    const r = lerp(10.5, 11.5, t);
    camera.position.set(Math.cos(a) * r, lerp(1.6, 3.2, t), Math.sin(a) * r);
    camLook.set(0, lerp(3.0, 3.6, t), 0);
  }, ease.inOut);

  // 5. Flip it upside down: data structure trees have the root on top
  typeText(l2, '…data trees grow down.', 34);
  typeText(l3, 'root on top · leaves at the bottom');
  const accent = new THREE.Color(accentColor());
  const leafGlow = new THREE.Color(0x8dff9e);
  const camStart = camera.position.clone();
  const camEnd = new THREE.Vector3(0, forest.PIVOT_Y, 13);
  const lookStart = camLook.clone();
  const lookEnd = new THREE.Vector3(0, forest.PIVOT_Y, 0);
  await tween(2600, t => {
    forest.pivot.rotation.z = Math.PI * t;
    camera.position.lerpVectors(camStart, camEnd, t);
    camLook.lerpVectors(lookStart, lookEnd, t);
    forest.tree.leafMat.emissive.copy(leafGlow).multiplyScalar(t * 0.45);
    forest.tree.bark.emissive.copy(accent).multiplyScalar(t * 0.6);
  });

  // 6. Hand over to the data explorer
  let data = null, error = null;
  try { data = await dataPromise; } catch (e) { error = e; }
  await openExplorer(data, error, { fromCinematic: true });
  skipping = false;
}

/* ============================================================
   Data loading
   ============================================================ */

async function fetchTree(impl) {
  const response = await fetch(DATA_FILES[impl]);
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  const data = await response.json();
  // give every node a stable id so D3 can animate it
  let id = 0;
  const stack = [data.root];
  while (stack.length) {
    const n = stack.pop();
    n._id = id++;
    if (n.children) for (const c of n.children) stack.push(c);
  }
  return data;
}

/* ============================================================
   SCENE 3 — EXPLORER (the data structure view)
   ============================================================ */

let svg, gZoom, gLinks, gNodes, gFx, zoom;
let positions = new Map();  // node id -> {x, y} from the last render

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

async function openExplorer(data, error, { fromCinematic = false } = {}) {
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

  // grow the data tree: root first, then its children branch out
  renderTree({ hideChildren: true });
  await wait(fromCinematic ? 500 : 250);
  renderTree();
}

function currentFolder() {
  return state.path[state.path.length - 1];
}

// The tree we draw: the path from root to the current folder, plus the current folder's children
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

const DY = 120;      // vertical space between path levels
const CELL_X = 104;  // grid spacing for the current folder's contents
const CELL_Y = 70;
const linkPath = d3.linkVertical().x(d => d.x).y(d => d.y);

// Path nodes go straight down; the current folder's contents wrap into a grid below it
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
  const dur = skipping ? 0 : 700;
  const stagger = (d, i) => (d.data && d.data.kind !== 'path' ? Math.min(i * 9, 450) : 0);

  // ---- links ----
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

  // ---- nodes ----
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
    g.append('circle').attr('class', 'hit').attr('r', 18);  // bigger, invisible click target
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

// Zoom so the last few levels of the path and the current folder's children fit
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

/* ---------- Clicking nodes ---------- */

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
  const implementation = state.implementation;
  const folder = currentFolder();
  let url;

  // Worst language ever?
  if (implementation === "B+ Tree") {
    let path = "";

    // exclude root
    for (const directory of state.path.slice(1)) {
      path += directory.name + '/';
    }

    url = "/backend/bplus_accesses?path=" + encodeURIComponent(path + file.name);
  }
  else {
    url = "/backend/splay_accesses?name=" + encodeURIComponent(file.name);
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
    setTimeout(() => runPulse(pts), skipping ? 0 : 450);
  }
}

function resetCounter() {
  state.totalNodesAccessed = 0;
  $('node-access-count').textContent = '0';
  $('last-access').textContent = 'Counter reset';
}

/* ---------- Effects ---------- */

// A glowing dot runs from the root down the path: one hop per node visited
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

/* ---------- Tooltip ---------- */

function showTip(event, d) {
  const tip = $('tooltip');
  if (d.data.kind === 'more') {
    tip.innerHTML = `<b>${d.data.count.toLocaleString()}</b> more items<br>Click to show more`;
  } else {
    const n = d.data.node;
    const kids = n.children || [];
    const folders = kids.filter(c => c.folder).length;
    tip.innerHTML = `<b>${n.name}</b><br>${n.folder ? 'Folder' : 'File'} · ${n.accesses} nodes to reach` +
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

/* ---------- Side panel ---------- */

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

/* ============================================================
   Buttons (called from index.html)
   ============================================================ */

function selectImplementation(implName) {
  if (state.phase !== 'home') return;
  state.implementation = implName;
  setImplementationClass(implName);
  const dataPromise = fetchTree(implName);
  dataPromise.catch(() => {});  // handled when the explorer opens

  if (!renderer) {
    dataPromise.then(d => openExplorer(d, null), e => openExplorer(null, e));
    return;
  }
  runCinematic(dataPromise);
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
  skipping = false;
  showScreen('home');
  if (renderer) {
    canvas.classList.remove('dim');
    enterHomeView();
  }
}

function skipIntro() {
  if (state.phase === 'cinematic') skipping = true;
}

window.selectImplementation = selectImplementation;
window.switchImplementation = switchImplementation;
window.goHome = goHome;
window.goBack = goBack;
window.resetCounter = resetCounter;

$('btn-skip').addEventListener('click', skipIntro);
document.addEventListener('keydown', e => {
  if (e.key === 'Escape') skipIntro();
  if (e.key === 'Backspace' && state.phase === 'explorer' && e.target === document.body) goBack();
});

let resizeTimer = null;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    if (state.phase === 'explorer' && svg) renderTree();
  }, 200);
});

/* ============================================================
   Start
   ============================================================ */

if (initGL()) {
  buildEarthScene();
  enterHomeView();
} else {
  canvas.style.display = 'none';
  document.body.style.background =
    'radial-gradient(ellipse at 50% 120%, #123a6b 0%, #03050b 60%)';
}