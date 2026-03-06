import React, { useState } from 'react';
import { ScannerProvider } from './context/ScannerContext';
import { TreeView } from './components/TreeView';
import { ScanFileUpload, ScanControls } from './components/Scans';
import { ScanResults } from './components/ScanResults';
import { UpdatesList, UpdateControls } from './components/Updates';
import './App.css';

function AppContent() {
  const [activeTab, setActiveTab] = useState('scans');

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
          {/* Tab Navigation */}
          <div className="tab-navigation">
            <button
              className={`tab-button ${activeTab === 'scans' ? 'active' : ''}`}
              onClick={() => setActiveTab('scans')}
            >
              Scans
            </button>
            <button
              className={`tab-button ${activeTab === 'updates' ? 'active' : ''}`}
              onClick={() => setActiveTab('updates')}
            >
              Updates
            </button>
          </div>

          <div className="content-grid">
            {/* Scans Tab */}
            {activeTab === 'scans' && (
              <div className="content-column">
                <ScanFileUpload />
                <ScanControls />
              </div>
            )}

            {/* Updates Tab */}
            {activeTab === 'updates' && (
              <div className="content-column">
                <UpdateControls />
              </div>
            )}
          </div>

          {/* Scan Results Full Width - only show for scans tab */}
          {activeTab === 'scans' && (
            <div className="results-section">
              <ScanResults />
            </div>
          )}

          {/* Updates List Full Width - only show for updates tab */}
          {activeTab === 'updates' && (
            <div className="results-section">
              <UpdatesList />
            </div>
          )}
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
