import React from 'react';
import { ScannerProvider } from './context/ScannerContext';
import { TreeView } from './components/TreeView';
import { ScanFileUpload, ScanControls } from './components/Scans';
import { ScanResults } from './components/ScanResults';
import './App.css';

function AppContent() {
  return (
    <div className="app-container">
      <header className="app-header">
        <div className="header-content">
          <img src="./logo_big.png" alt="Ahnalytics Logo" className="logo-big" />
        </div>
      </header>

      <main className="app-main">
        {/* Left Sidebar with Tree View */}
        <div className="sidebar">
          <TreeView />
        </div>

        {/* Right Content Area */}
        <div className="content-area">
          <div className="content-grid">
            {/* Scan Controls Column */}
            <div className="content-column">
              <ScanFileUpload />
              <ScanControls />
            </div>
          </div>

          {/* Scan Results Full Width */}
          <div className="results-section">
            <ScanResults />
          </div>
        </div>
      </main>

      <footer className="app-footer">
        <p>AhnalyticScanner - Source Code Scanner</p>
      </footer>
    </div>
  );
}

function App() {
  return (
    <ScannerProvider>
      <AppContent />
    </ScannerProvider>
  );
}

export default App;
