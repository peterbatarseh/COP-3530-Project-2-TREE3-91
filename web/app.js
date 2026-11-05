// Data comes from the C++ ExportWeb program, which writes these files.
const DATA_FILES = {
  'B+ Tree': 'data/bplus.json',
  'Splay Tree': 'data/splay.json'
};

let state = {
  implementation: 'B+ Tree',
  root: null,          // the loaded tree for the current implementation
  path: [],            // folders from root to the current folder
  totalNodesAccessed: 0
};

async function loadImplementation(implName) {
  const display = document.getElementById('file-display');
  try {
    const response = await fetch(DATA_FILES[implName]);
    if (!response.ok) throw new Error(response.status);
    const data = await response.json();

    state.implementation = implName;
    state.root = data.root;
    state.path = [data.root];
    state.totalNodesAccessed = data.root.accesses;
    updateUI();
  } catch (err) {
    display.innerHTML =
      '<p class="empty-msg">Could not load ' + DATA_FILES[implName] +
      '. Open the page with Live Server / Live Preview (not by double-clicking), ' +
      'and run the ExportWeb program if the file is missing.</p>';
    console.error(err);
  }
}

function selectImplementation(implName) {
  document.getElementById('btn-bplus').classList.toggle('active', implName === 'B+ Tree');
  document.getElementById('btn-splay').classList.toggle('active', implName === 'Splay Tree');
  loadImplementation(implName);
}

function currentFolder() {
  return state.path[state.path.length - 1];
}

function openFolder(index) {
  const folder = currentFolder().children[index];
  state.path.push(folder);
  state.totalNodesAccessed += folder.accesses;
  updateUI();
}

function goBack() {
  if (state.path.length > 1) {
    state.path.pop();
    state.totalNodesAccessed += currentFolder().accesses;
    updateUI();
  }
}

function resetCounter() {
  state.totalNodesAccessed = 0;
  updateUI();
}

function updateUI() {
  const folder = currentFolder();
  const children = folder.children || [];

  document.getElementById('current-impl').textContent = state.implementation;
  document.getElementById('current-dir').textContent =
    '/' + state.path.map(f => f.name).join('/');
  document.getElementById('subfolder-count').textContent = children.filter(c => c.folder).length;
  document.getElementById('file-count').textContent = children.filter(c => !c.folder).length;
  document.getElementById('node-access-count').textContent = state.totalNodesAccessed;
  document.getElementById('btn-back').disabled = state.path.length <= 1;

  const display = document.getElementById('file-display');
  display.innerHTML = '';

  if (children.length === 0) {
    display.innerHTML = '<p class="empty-msg">This folder is empty.</p>';
    return;
  }

  children.forEach((child, i) => {
    const item = document.createElement(child.folder ? 'button' : 'div');
    item.className = 'item' + (child.folder ? ' is-folder' : '');

    const icon = document.createElement('div');
    icon.className = child.folder ? 'folder-icon' : 'file-icon';

    const label = document.createElement('span');
    label.textContent = child.name;

    item.append(icon, label);
    if (child.folder) item.onclick = () => openFolder(i);
    display.appendChild(item);
  });
}

document.addEventListener('DOMContentLoaded', () => loadImplementation('B+ Tree'));
