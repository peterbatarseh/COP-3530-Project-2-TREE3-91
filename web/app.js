let state = {
  implementation: 'B+ Tree',
  currentDirectory: '/root',
  subdirectoryCount: 0,
  fileCount: 0,
  totalNodesAccessed: 0
};

function selectImplementation(implName) {
  state.implementation = implName;

  document.getElementById('btn-bplus').classList.toggle('active', implName === 'B+ Tree');
  document.getElementById('btn-splay').classList.toggle('active', implName === 'Splay Tree');

  updateUI();
}

function updateUI() {
  document.getElementById('current-impl').textContent = state.implementation;
  document.getElementById('current-dir').textContent = state.currentDirectory;
  document.getElementById('subfolder-count').textContent = state.subdirectoryCount;
  document.getElementById('file-count').textContent = state.fileCount;
  document.getElementById('node-access-count').textContent = state.totalNodesAccessed;
}

document.addEventListener('DOMContentLoaded', updateUI);