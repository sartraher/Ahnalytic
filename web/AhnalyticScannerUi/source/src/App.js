import React from 'react';
import { ScannerProvider } from './context/ScannerContext';
import { GroupsList, GroupForm } from './components/Groups';
import { ProjectsList, ProjectForm } from './components/Projects';
import { VersionsList, VersionForm } from './components/Versions';
import { ScansList, ScanForm, ScanFileUpload, ScanControls } from './components/Scans';
import { ScanResults } from './components/ScanResults';
import './App.css';

function AppContent() {
  return (
    <div className="app-container">
      <header className="app-header">
        <div className="header-content">
          <img src="/logo_big.png" alt="Ahnalytics Logo" className="logo-big" />
        </div>
      </header>

      <main className="app-main">
        <div className="sidebar">
          <nav className="nav-section">
            <GroupForm />
            <GroupsList />
          </nav>
        </div>

        <div className="content-area">
          <div className="content-grid">
            {/* Projects Column */}
            <div className="content-column">
              <ProjectForm />
              <ProjectsList />
            </div>

            {/* Versions Column */}
            <div className="content-column">
              <VersionForm />
              <VersionsList />
            </div>

            {/* Scans Column */}
            <div className="content-column">
              <ScanForm />
              <ScansList />
            </div>
          </div>

          {/* Details Area */}
          <div className="details-section">
            <div className="details-grid">
              <div className="details-column">
                <ScanFileUpload />
                <ScanControls />
              </div>

              <div className="details-column">
                <ScanResults />
              </div>
            </div>
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
