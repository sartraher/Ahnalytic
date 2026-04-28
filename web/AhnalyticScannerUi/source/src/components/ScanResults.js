import React, { useState, useEffect, useRef, useMemo } from 'react';
import { useScanner } from '../context/ScannerContext';
import { DiffViewer } from './DiffViewer';
import api from '../services/api';
import '../styles/components.css';
import '../styles/configuration.css';

// Helper function to convert numeric status codes to status names
const getStatusName = (status) => {
  if (typeof status === 'string') return status;
  const statusMap = {
    0: 'idle',
    1: 'preparing',
    2: 'ready',
    3: 'started',
    4: 'running',
    5: 'aborted',
    6: 'finished',
  };
  return statusMap[status] || 'unknown';
};

// Helper function to build file tree from results
const buildFileTree = (results) => {
  const tree = {};

  results.forEach((result) => {
    if (result.searchFile) {
      // Split on both forward and backslashes
      const parts = result.searchFile.split(/[\/\\]/).filter(p => p);
      let current = tree;

      parts.forEach((part, index) => {
        if (!current[part]) {
          current[part] = {
            name: part,
            path: parts.slice(0, index + 1).join('/'),
            isFile: index === parts.length - 1,
            children: {},
            resultCount: 0,
          };
        }
        if (index === parts.length - 1) {
          current[part].resultCount += result.resultSets?.length || 0;
        }
        current = current[part].children;
      });
    }
  });

  return tree;
};

// Configuration viewer component
const ConfigurationViewer = ({ config, onEdit, onClose }) => {
  const getTypeColor = (type) => {
    switch (type) {
      case 'Content': return '#3498db';
      case '3rdParty': return '#e74c3c';
      case 'Ignore': return '#95a5a6';
      default: return '#34495e';
    }
  };

  return (
    <div className="config-viewer">
      <div className="config-header">
        <div className="config-title">
          <span className="config-name">{config.data.name}</span>
          <span className="config-type-badge" style={{ backgroundColor: getTypeColor(config.data.data.type) }}>
            {config.data.data.type}
          </span>
        </div>
        <button className="btn-icon" onClick={onClose} title="Close">✕</button>
      </div>

      <div className="config-content">
        <div className="config-section">
          <h5>Target</h5>
          <div className="config-value code">{config.data.data.target}</div>
        </div>

        {config.data.data.type === '3rdParty' && config.data.data.thirdParty && (
          <div className="config-section">
            <h5>Third Party Details</h5>
            <div className="config-row">
              <span className="label">Vendor:</span>
              <span className="value">{config.data.data.thirdParty.vendor}</span>
            </div>
            <div className="config-row">
              <span className="label">Product:</span>
              <span className="value">{config.data.data.thirdParty.product}</span>
            </div>
            <div className="config-row">
              <span className="label">Version:</span>
              <span className="value">{config.data.data.thirdParty.version}</span>
            </div>
          </div>
        )}

        {config.data.data.type === 'Content' && config.data.data.resultFilters && config.data.data.resultFilters.length > 0 && (
          <div className="config-section">
            <h5>Result Filters ({config.data.data.resultFilters.length})</h5>
            {config.data.data.resultFilters.map((filter, idx) => (
              <div key={idx} className="config-subsection">
                <div className="config-row">
                  <span className="label">DB File:</span>
                  <span className="value code">{filter.dbFile}</span>
                </div>
                <div className="config-row">
                  <span className="label">Search File:</span>
                  <span className="value code">{filter.searchFile}</span>
                </div>
                <div className="config-row">
                  <span className="label">Reason:</span>
                  <span className="value">{filter.reason}</span>
                </div>
              </div>
            ))}
          </div>
        )}

        {config.data.data.cveConfigs && config.data.data.cveConfigs.length > 0 && (
          <div className="config-section">
            <h5>CVE Configurations ({config.data.data.cveConfigs.length})</h5>
            {config.data.data.cveConfigs.map((cve, idx) => (
              <div key={idx} className="config-subsection">
                <div className="config-row">
                  <span className="label">ID:</span>
                  <span className="value">{cve.id}</span>
                </div>
                <div className="config-row">
                  <span className="label">Status:</span>
                  <span className="value">{cve.status}</span>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>

      <div className="config-footer">
        <button className="btn-primary" onClick={() => onEdit(config)}>Edit Configuration</button>
      </div>
    </div>
  );
};

// Configuration editor component for creating/editing configurations
const ConfigurationEditorForm = ({ filePath, config, onSave, onCancel, isNew = false }) => {
  // Extract name from path (filename or folder name without path)
  const getNameFromPath = (path) => {
    if (!path) return '';
    const parts = path.split(/[\\\\\\/]/);
    return parts[parts.length - 1] || '';
  };

  // Determine target based on whether it's a file or folder
  const getTargetFromPath = (path) => {
    if (!path) return '.';
    // If path contains a file extension, it's likely a file - use just filename
    // Otherwise it's a folder, use '.'
    const parts = path.split(/[\\\\\\/]/);
    const lastPart = parts[parts.length - 1];
    // Check if last part has an extension (contains a dot)
    return lastPart.includes('.') ? lastPart : '.';
  };

  const autoName = getNameFromPath(filePath);
  const autoTarget = getTargetFromPath(filePath);

  const [formData, setFormData] = useState(config || {
    name: autoName,
    type: 'Content',
    target: autoTarget,
    resultFilters: [],
    thirdParty: null,
    cveConfigs: []
  });

  const handleDataChange = (field, value) => {
    setFormData(prev => ({
      ...prev,
      [field]: value
    }));
  };

  const handleSave = () => {
    onSave(filePath, formData);
  };

  return (
    <div className="config-editor-inline">
      <div className="config-editor-header">
        <h3>{isNew ? 'Create New Configuration' : 'Edit Configuration'}</h3>
        <button className="btn-icon" onClick={onCancel} title="Close">✕</button>
      </div>

      <div className="config-editor-content\">
          {/* Show info about the selection */}
          <div className="form-group">
            <p style={{ fontSize: '0.9rem', color: '#555', marginBottom: '1rem' }}>
              <strong>Path:</strong> {filePath}<br />
              <strong>Configuration Name:</strong> {formData.name}<br />
              <strong>Target:</strong> {formData.target}
            </p>
          </div>

          <div className="form-group">
            <label>Type</label>
            <select
              value={formData.type}
              onChange={(e) => handleDataChange('type', e.target.value)}
              className="form-input"
            >
              <option value="Content">Content</option>
              <option value="3rdParty">3rd Party</option>
              <option value="Ignore">Ignore</option>
            </select>
          </div>

          {formData.type === '3rdParty' && (
            <>
              <div className="form-group">
                <label>Vendor</label>
                <input
                  type="text"
                  value={formData.thirdParty?.vendor || ''}
                  onChange={(e) => handleDataChange('thirdParty', {
                    ...formData.thirdParty,
                    vendor: e.target.value
                  })}
                  className="form-input"
                />
              </div>
              <div className="form-group">
                <label>Product</label>
                <input
                  type="text"
                  value={formData.thirdParty?.product || ''}
                  onChange={(e) => handleDataChange('thirdParty', {
                    ...formData.thirdParty,
                    product: e.target.value
                  })}
                  className="form-input"
                />
              </div>
              <div className="form-group">
                <label>Version</label>
                <input
                  type="text"
                  value={formData.thirdParty?.version || ''}
                  onChange={(e) => handleDataChange('thirdParty', {
                    ...formData.thirdParty,
                    version: e.target.value
                  })}
                  className="form-input"
                />
              </div>
            </>
          )}
        </div>

      <div className="config-editor-footer">
        <button className="btn-secondary" onClick={onCancel}>Cancel</button>
        <button className="btn-primary" onClick={handleSave}>
          {isNew ? 'Create Configuration' : 'Save Configuration'}
        </button>
      </div>
    </div>
  );
};

// File tree renderer - properly shows folders and files in tree structure
const FileTreeRenderer = ({ tree, selectedPath, onSelect, expandedPaths, onToggleExpand, onSelectConfig, onAddConfig }) => {
  if (!tree) {
    return <div className="file-tree-container"><p className="empty-message">No file tree available</p></div>;
  }

  const renderNode = (node, path = '', isRoot = false) => {
    const nodePath = path ? `${path}/${node.name}` : node.name;
    const isSelected = selectedPath === nodePath;
    // Auto-expand root node
    const isExpanded = isRoot || expandedPaths[nodePath] || false;
    const hasChildren = (node.folders && node.folders.length > 0) || (node.files && node.files.length > 0) || (node.configurations && node.configurations.length > 0);

    // For empty root node, render children directly without container
    if (isRoot && !node.name) {
      return (
        <div className="file-tree-children">
          {/* Render subfolders */}
          {node.folders && node.folders.map((folder) => renderNode(folder, nodePath))}
          
          {/* Render files */}
          {node.files && node.files.map((file) => (
            <div key={`${nodePath}/${file.name}`} className="file-tree-node">
              <div
                className={`file-tree-node-content ${isSelected === `${nodePath}/${file.name}` ? 'active' : ''} file`}
                onClick={(e) => {
                  e.stopPropagation();
                  onSelect(`${nodePath}/${file.name}`);
                }}
              >
                <span className="file-tree-expander empty"></span>
                <span className="file-tree-icon">📄</span>
                <span className="file-tree-label">{file.name}</span>
                {file.configurations && file.configurations.length > 0 && (
                  <span className="config-count">⚙️ {file.configurations.length}</span>
                )}
              </div>
            </div>
          ))}

          {/* Render configurations */}
          {node.configurations && node.configurations.map((config) => (
            <div key={`${nodePath}/config-${config.name}`} className="file-tree-node config-node">
              <div
                className="file-tree-node-content config"
                onClick={(e) => {
                  e.stopPropagation();
                  onSelectConfig({ path: nodePath, config });
                }}
              >
                <span className="file-tree-expander empty"></span>
                <span className="file-tree-icon">⚙️</span>
                <span className="file-tree-label">{config.name}</span>
                <span className="config-type-badge-small" style={{
                  backgroundColor: config.data.type === 'Content' ? '#3498db' : (config.data.type === '3rdParty' ? '#e74c3c' : '#95a5a6'),
                  color: 'white',
                  fontSize: '0.75rem',
                  padding: '2px 6px',
                  borderRadius: '3px',
                  marginLeft: '4px'
                }}>
                  {config.data.type}
                </span>
              </div>
            </div>
          ))}
        </div>
      );
    }

    return (
      <div key={nodePath} className="file-tree-node">
        <div
          className={`file-tree-node-content ${isSelected ? 'active' : ''} folder`}
          onClick={(e) => {
            e.stopPropagation();
            onSelect(nodePath);
            onToggleExpand(nodePath);
          }}
        >
          {hasChildren ? (
            <span className="file-tree-expander">
              {isExpanded ? '▼' : '▶'}
            </span>
          ) : (
            <span className="file-tree-expander empty"></span>
          )}
          <span className="file-tree-icon">📁</span>
          <span className="file-tree-label">{node.name}</span>
          {node.configurations && node.configurations.length > 0 && (
            <span className="config-count">⚙️ {node.configurations.length}</span>
          )}
        </div>
        {isExpanded && hasChildren && (
          <div className="file-tree-children">
            {/* Render subfolders */}
            {node.folders && node.folders.map((folder) => renderNode(folder, nodePath))}
            
            {/* Render files */}
            {node.files && node.files.map((file) => (
              <div key={`${nodePath}/${file.name}`} className="file-tree-node">
                <div
                  className={`file-tree-node-content ${isSelected === `${nodePath}/${file.name}` ? 'active' : ''} file`}
                  onClick={(e) => {
                    e.stopPropagation();
                    onSelect(`${nodePath}/${file.name}`);
                  }}
                >
                  <span className="file-tree-expander empty"></span>
                  <span className="file-tree-icon">📄</span>
                  <span className="file-tree-label">{file.name}</span>
                  {file.configurations && file.configurations.length > 0 && (
                    <span className="config-count">⚙️ {file.configurations.length}</span>
                  )}
                </div>
              </div>
            ))}

            {/* Render configurations */}
            {node.configurations && node.configurations.map((config) => (
              <div key={`${nodePath}/config-${config.name}`} className="file-tree-node config-node">
                <div
                  className="file-tree-node-content config"
                  onClick={(e) => {
                    e.stopPropagation();
                    onSelectConfig({ path: nodePath, config });
                  }}
                >
                  <span className="file-tree-expander empty"></span>
                  <span className="file-tree-icon">⚙️</span>
                  <span className="file-tree-label">{config.name}</span>
                  <span className="config-type-badge-small" style={{
                    backgroundColor: config.data.type === 'Content' ? '#3498db' : (config.data.type === '3rdParty' ? '#e74c3c' : '#95a5a6'),
                    color: 'white',
                    fontSize: '0.75rem',
                    padding: '2px 6px',
                    borderRadius: '3px',
                    marginLeft: '4px'
                  }}>
                    {config.data.type}
                  </span>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    );
  };

  return (
    <div className="file-tree-container">
      {renderNode(tree, '', true)}
    </div>
  );
};

// Memoized progress bars component - updates independently
const ProgressBarsSection = React.memo(({ status, results, finishedCount, maxCount, deepFinishedCount, deepMaxCount }) => {
  const progress = maxCount > 0 ? (finishedCount / maxCount) * 100 : 0;
  const deepProgress = deepMaxCount > 0 ? (deepFinishedCount / deepMaxCount) * 100 : 0;

  return (
    <div className="scan-results-header">
      <h3>Scan Results</h3>

      {status && (
        <div className="scan-header">
          <span className={`status-badge status-${getStatusName(status)}`}>{getStatusName(status).toUpperCase()}</span>
          <span className="result-count">{results.length} matches found</span>
        </div>
      )}

      {/* Progress Bars */}
      {maxCount > 0 && (
        <div className="progress-container">
          <div className="progress-label">Fingerprint databases Scanned</div>
          <div className="progress-bar-wrapper">
            <div className="progress-bar">
              <div className="progress-fill" style={{ width: `${progress}%` }}></div>
            </div>
          </div>
          <span className="progress-text">
            {finishedCount} / {maxCount}
          </span>
        </div>
      )}

      {deepMaxCount > 0 && (
        <div className="progress-container">
          <div className="progress-label">Deep Scan Progress</div>
          <div className="progress-bar-wrapper">
            <div className="progress-bar deep">
              <div className="progress-fill" style={{ width: `${deepProgress}%` }}></div>
            </div>
          </div>
          <span className="progress-text">
            {deepFinishedCount} / {deepMaxCount}
          </span>
        </div>
      )}
    </div>
  );
});

ProgressBarsSection.displayName = 'ProgressBarsSection';

// Memoized results list section
const ResultsListSection = React.memo(({ 
  filteredResults, 
  selectedFilePath, 
  fileTree, 
  expandedPaths, 
  expandedResultId, 
  onToggleExpand, 
  onSelectFile, 
  onToggleResultExpand,
  scrollContainerRef,
  groupId,
  projectId,
  versionId,
  scanId,
  prescanLoading,
  selectedConfig,
  onSelectConfig,
  onCloseConfig,
  results,
  onAddConfig,
  configPathBeingEdited,
  editingConfig,
  onSaveConfig,
  onCancelEditConfig
}) => {
  const getSeverityColor = (matchCount) => {
    if (matchCount === 0) return 'green';
    if (matchCount < 5) return 'yellow';
    if (matchCount < 20) return 'orange';
    return 'red';
  };

  return (
    <div className="scan-results-content">
      {/* Configuration Panel */}
      {selectedConfig && (
        <div className="config-panel">
          <ConfigurationViewer
            config={selectedConfig.config}
            onEdit={(config) => {}}
            onClose={onCloseConfig}
          />
        </div>
      )}

      {/* File Tree Panel */}
      <div className="file-tree-panel">
        <div className="file-tree-header">
          <h4>Files {selectedConfig && <span style={{fontSize: '0.85rem', fontWeight: 'normal'}}>- Config Selected</span>}</h4>
          {prescanLoading && <span className="loading-indicator">Loading...</span>}
          {(selectedFilePath || selectedConfig) && (
            <button
              className="btn-icon"
              onClick={() => {
                onSelectFile(null);
                onCloseConfig();
              }}
              title="Clear selection"
            >
              ✕
            </button>
          )}
        </div>
        <FileTreeRenderer
          tree={fileTree}
          selectedPath={selectedFilePath}
          onSelect={onSelectFile}
          expandedPaths={expandedPaths}
          onToggleExpand={onToggleExpand}
          onSelectConfig={onSelectConfig}
          onAddConfig={onAddConfig}
        />
      </div>

      {/* Results/Configuration Panel */}
      <div className="results-panel">
        {/* Configuration Details Panel */}
        {selectedConfig && (
          <div className="config-details-panel">
            <ConfigurationViewer
              config={selectedConfig.config}
              onEdit={(config) => {}}
              onClose={onCloseConfig}
            />
          </div>
        )}

        {/* Add/Edit Configuration Panel */}
        {!selectedConfig && selectedFilePath && (
          <div className="add-config-panel">
            {!configPathBeingEdited ? (
              <div className="empty-state">
                <div className="empty-icon">⚙️</div>
                <h4>Add Configuration</h4>
                <p>Create a new configuration for this file/folder</p>
                <button
                  className="btn-primary"
                  onClick={() => onAddConfig(selectedFilePath)}
                >
                  + Add Configuration
                </button>
              </div>
            ) : (
              <ConfigurationEditorForm
                filePath={configPathBeingEdited}
                config={editingConfig}
                onSave={onSaveConfig}
                onCancel={onCancelEditConfig}
                isNew={!editingConfig}
              />
            )}
          </div>
        )}

        {/* Scan Results Display */}
        {results.length === 0 && !selectedConfig && !selectedFilePath ? (
          <p className="empty-message">No matches found in this scan</p>
        ) : results.length === 0 && !selectedConfig ? (
          <p className="empty-message">No results for selected file</p>
        ) : filteredResults.length === 0 && results.length > 0 ? (
          <p className="empty-message">No results for selected file</p>
        ) : (
          <div className="results-container" ref={scrollContainerRef}>
            {filteredResults.map((result, idx) => (
              <div
                key={idx}
                className="result-item"
                onClick={() => onToggleResultExpand(idx)}
              >
                <div className="result-header">
                  <div className="result-title-section">
                    <span className={`severity-indicator severity-${getSeverityColor(result.resultSets?.length || 0)}`}>
                      ●
                    </span>
                    <span className="result-title">
                      {result.sourceDb} - {result.sourceFile}
                    </span>
                  </div>
                  <div className="result-meta">
                    <span className="match-count">
                      {result.resultSets?.length || 0} matches
                    </span>
                    <button className="btn-icon">
                      {expandedResultId === idx ? '▼' : '▶'}
                    </button>
                  </div>
                </div>

                {expandedResultId === idx && (
                  <div className="result-details">
                    {result.licence && (
                      <div className="detail-section">
                        <h5>License</h5>
                        <div className="detail-row">
                          <span className="label">Type:</span>
                          <span className="value">{result.licence}</span>
                        </div>
                      </div>
                    )}

                    <div className="detail-section">
                      <h5>Source Information</h5>
                      <div className="detail-row">
                        <span className="label">Database:</span>
                        <span className="value">{result.sourceDb}</span>
                      </div>
                      <div className="detail-row">
                        <span className="label">File:</span>
                        <span className="value code">{result.sourceFile}</span>
                      </div>
                      {result.sourceRevision && (
                        <div className="detail-row">
                          <span className="label">Revision:</span>
                          <span className="value code">{result.sourceRevision}</span>
                        </div>
                      )}
                      {result.sourceInternalId && (
                        <div className="detail-row">
                          <span className="label">Internal ID:</span>
                          <span className="value code">{result.sourceInternalId}</span>
                        </div>
                      )}
                    </div>

                    <div className="detail-section">
                      <h5>Scan File</h5>
                      <div className="detail-row">
                        <span className="label">File:</span>
                        <span className="value code">{result.searchFile}</span>
                      </div>
                    </div>

                    {result.resultSets && result.resultSets.length > 0 && (
                      <div className="detail-section">
                        <h5>Match Locations ({result.resultSets.length})</h5>
                        
                        {/* Show Diff Viewer with lazy-loaded content */}
                        {result.elementIndex !== undefined && (
                          <div className="detail-subsection">
                            <DiffViewer
                              elementIndex={result.elementIndex}
                              resultSets={result.resultSets}
                              groupId={groupId}
                              projectId={projectId}
                              versionId={versionId}
                              scanId={scanId}
                            />
                          </div>
                        )}

                        {/* Show match locations list */}
                        <div className="matches-list">
                          {result.resultSets.map((match, matchIdx) => (
                            <div key={matchIdx} className="match-item">
                              <div className="match-header">
                                Match {matchIdx + 1}
                              </div>
                              <div className="match-locations">
                                <div className="location">
                                  <span className="location-label">Source Location:</span>
                                  <span className="location-value">
                                    {match.baseStart} - {match.baseEnd}
                                  </span>
                                </div>
                                <div className="location">
                                  <span className="location-label">Search Location:</span>
                                  <span className="location-value">
                                    {match.searchStart} - {match.searchEnd}
                                  </span>
                                </div>
                              </div>
                            </div>
                          ))}
                        </div>
                      </div>
                    )}
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
});

ResultsListSection.displayName = 'ResultsListSection';

export const ScanResults = () => {
  const { currentGroup, currentProject, currentVersion, currentScan, scanInfo, loading, error, loadScanInfo } = useScanner();
  const [expandedResultId, setExpandedResultId] = useState(null);
  const [selectedFilePath, setSelectedFilePath] = useState(null);
  const [expandedPaths, setExpandedPaths] = useState({});
  const [apiFileTree, setApiFileTree] = useState(null);
  const [prescanLoading, setPrescanLoading] = useState(false);
  const [selectedConfig, setSelectedConfig] = useState(null);
  const [configPathBeingEdited, setConfigPathBeingEdited] = useState(null);
  const [editingConfig, setEditingConfig] = useState(null);
  const scrollContainerRef = useRef(null);
  const scrollPositionRef = useRef(0);
  const treePollingRef = useRef(null);
  
  // Separate progress state to avoid re-rendering the entire component during polling
  const [progress, setProgress] = useState({ finishedCount: 0, maxCount: 0, deepFinishedCount: 0, deepMaxCount: 0 });
  const progressTimeoutRef = useRef(null);

  // Load scan info automatically when a scan is selected
  useEffect(() => {
    if (currentGroup != null && currentProject != null && currentVersion != null && currentScan != null) {
      loadScanInfo(currentGroup, currentProject, currentVersion, currentScan);
    }
  }, [currentGroup, currentProject, currentVersion, currentScan, loadScanInfo]);

  // Load file tree when scan is selected (without prescan)
  useEffect(() => {
    if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
      return;
    }

    const loadTree = async () => {
      try {
        // Get the file tree without calling prescan
        const tree = await api.getFileTree(currentGroup, currentProject, currentVersion, currentScan);
        setApiFileTree(tree);
      } catch (err) {
        console.error('Failed to load file tree:', err);
        setApiFileTree(null);
      }
    };

    loadTree();
  }, [currentGroup, currentProject, currentVersion, currentScan]);

  // Poll file tree while prescan is PREPARING (status 1), stop when READY (status 2) or tree is populated
  useEffect(() => {
    if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
      return;
    }

    // Only poll if status is PREPARING (1)
    const isPreparing = scanInfo?.status === 1;
    // Check if tree has actual content (not just empty root)
    const hasTreeData = apiFileTree && apiFileTree.folders && apiFileTree.folders.length > 0;
    
    console.log(`🔍 Status: ${getStatusName(scanInfo?.status)} (${scanInfo?.status}), HasTreeData: ${hasTreeData}, Polling: ${isPreparing && !hasTreeData}`);

    // Clean up any existing polling
    if (treePollingRef.current) {
      clearInterval(treePollingRef.current);
      treePollingRef.current = null;
    }

    // Stop if not preparing OR if we already have tree data
    if (!isPreparing || hasTreeData) {
      console.log(`🛑 Stopping polling - Status: ${isPreparing ? 'PREPARING' : 'NOT PREPARING'}, HasData: ${hasTreeData}`);
      return;
    }

    const pollFileTree = async () => {
      try {
        const tree = await api.getFileTree(currentGroup, currentProject, currentVersion, currentScan);
        console.log(`📋 Polled tree:`, tree);
        setApiFileTree(tree);
        
        // Check if we now have data - if so, next effect run will stop polling
        if (tree && tree.folders && tree.folders.length > 0) {
          console.log('✅ Tree data received, polling will stop next check');
        }
      } catch (err) {
        console.error('❌ Polling error:', err);
      }
    };

    // Immediate poll
    console.log('🚀 Starting file tree polling (PREPARING)');
    pollFileTree();
    
    // Start polling interval - every 1 second while PREPARING
    treePollingRef.current = setInterval(pollFileTree, 1000);

    return () => {
      if (treePollingRef.current) {
        clearInterval(treePollingRef.current);
        treePollingRef.current = null;
      }
    };
  }, [currentGroup, currentProject, currentVersion, currentScan, scanInfo?.status, apiFileTree?.folders?.length]);

  // Initialize progress state from scanInfo when it loads
  useEffect(() => {
    if (scanInfo) {
      setProgress({
        finishedCount: scanInfo.finishedCount || 0,
        maxCount: scanInfo.maxCount || 0,
        deepFinishedCount: scanInfo.deepFinishedCount || 0,
        deepMaxCount: scanInfo.deepMaxCount || 0,
      });
    }
  }, [scanInfo?.results?.length]); // Only update when results length changes, not on every poll

  // Save scroll position before updates
  useEffect(() => {
    const container = scrollContainerRef.current;
    if (container) {
      const handleScroll = () => {
        scrollPositionRef.current = container.scrollTop;
      };
      container.addEventListener('scroll', handleScroll);
      return () => container.removeEventListener('scroll', handleScroll);
    }
  }, []);

  // Poll for progress updates only - uses a separate API call to avoid full re-render
  useEffect(() => {
    if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
      if (progressTimeoutRef.current) {
        clearTimeout(progressTimeoutRef.current);
        progressTimeoutRef.current = null;
      }
      return;
    }

    // Check if scan is still running based on current scanInfo (Started = 3, Running = 4)
    const isRunning = scanInfo && (scanInfo.status === 3 || scanInfo.status === 4 || scanInfo.status === 'started' || scanInfo.status === 'running');
    
    if (!isRunning) {
      if (progressTimeoutRef.current) {
        clearTimeout(progressTimeoutRef.current);
        progressTimeoutRef.current = null;
      }
      return;
    }

    const pollProgress = async () => {
      try {
        // Call the API directly to avoid setting loading state
        const data = await api.getScanInfo(currentGroup, currentProject, currentVersion, currentScan);
        if (data) {
          // Only update progress state, not full scanInfo, to avoid re-rendering results
          setProgress({
            finishedCount: data.finishedCount || 0,
            maxCount: data.maxCount || 0,
            deepFinishedCount: data.deepFinishedCount || 0,
            deepMaxCount: data.deepMaxCount || 0,
          });
        }
      } catch (e) {
        // Silently fail on polling errors
      }
    };

    // Initial poll
    pollProgress();

    // Set up polling interval
    progressTimeoutRef.current = setInterval(pollProgress, 1000);

    return () => {
      if (progressTimeoutRef.current) {
        clearInterval(progressTimeoutRef.current);
        progressTimeoutRef.current = null;
      }
    };
  }, [currentGroup, currentProject, currentVersion, currentScan, scanInfo]);

  // Use the API file tree (or build from results as fallback)
  const fileTree = useMemo(() => {
    if (apiFileTree) return apiFileTree;
    if (!scanInfo || !scanInfo.results) return null;
    return buildFileTree(scanInfo.results);
  }, [apiFileTree, scanInfo]);

  // Filter results based on selected file path
  const filteredResults = useMemo(() => {
    if (!scanInfo || !scanInfo.results) return [];
    if (!selectedFilePath) return scanInfo.results;
    return scanInfo.results.filter((result) => {
      // Normalize backslashes to forward slashes for comparison
      const normalizedPath = result.searchFile.replace(/\\/g, '/');
      return normalizedPath === selectedFilePath;
    });
  }, [scanInfo, selectedFilePath]);

  // Restore scroll position after filtered results change (but not on every poll)
  useEffect(() => {
    const container = scrollContainerRef.current;
    if (container) {
      // Use setTimeout to defer scroll restoration to after render
      const timer = setTimeout(() => {
        container.scrollTop = scrollPositionRef.current;
      }, 0);
      return () => clearTimeout(timer);
    }
  }, [filteredResults]);

  const handleToggleExpand = (path) => {
    setExpandedPaths((prev) => ({
      ...prev,
      [path]: !prev[path],
    }));
  };

  const handleSelectFile = (path) => {
    setSelectedFilePath(selectedFilePath === path ? null : path);
    // Auto-expand first result when selecting a file
    if (selectedFilePath !== path) {
      setExpandedResultId(0);
    } else {
      setExpandedResultId(null);
    }
  };

  const handleAddConfig = (filePath) => {
    setConfigPathBeingEdited(filePath);
    setEditingConfig(null); // null means creating new
  };

  const handleSaveConfig = async (filePath, configData) => {
    try {
      // Build flat JSON with path and all config data
      const flatConfig = {
        path: filePath,
        name: configData.name,
        type: configData.type,
        target: configData.target,
        resultFilters: configData.resultFilters || [],
        cveConfigs: configData.cveConfigs || []
      };

      // Add thirdParty if type is 3rdParty
      if (configData.type === '3rdParty' && configData.thirdParty) {
        flatConfig.thirdParty = configData.thirdParty;
      }

      console.log('Saving configuration:', flatConfig);
      
      // Send to API
      await api.saveConfiguration(currentGroup, currentProject, currentVersion, currentScan, flatConfig);
      
      // Close editor on success
      setConfigPathBeingEdited(null);
      setEditingConfig(null);
      
      // Optionally reload tree or show success message
      console.log('Configuration saved successfully');
    } catch (err) {
      console.error('Failed to save configuration:', err);
      alert('Failed to save configuration: ' + err.message);
    }
  };

  const handleCancelEditConfig = () => {
    setConfigPathBeingEdited(null);
    setEditingConfig(null);
  };

  if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
    return (
      <div className="component-container">
        <p className="empty-message">Select a scan to view results</p>
      </div>
    );
  }

  if (!scanInfo) {
    return (
      <div className="component-container">
        <p className="empty-message">No scan info available</p>
      </div>
    );
  }

  if (loading) {
    return <div className="loading">Loading scan results...</div>;
  }

  const { results = [], status } = scanInfo;

  const toggleExpanded = (id) => {
    setExpandedResultId(expandedResultId === id ? null : id);
  };

  return (
    <div className="scan-results-wrapper">
      {error && <div className="error-message">{error}</div>}
      
      <ProgressBarsSection
        status={status}
        results={results}
        finishedCount={progress.finishedCount}
        maxCount={progress.maxCount}
        deepFinishedCount={progress.deepFinishedCount}
        deepMaxCount={progress.deepMaxCount}
      />

      <ResultsListSection
        filteredResults={filteredResults}
        selectedFilePath={selectedFilePath}
        fileTree={fileTree}
        expandedPaths={expandedPaths}
        expandedResultId={expandedResultId}
        onToggleExpand={handleToggleExpand}
        onSelectFile={handleSelectFile}
        onToggleResultExpand={toggleExpanded}
        scrollContainerRef={scrollContainerRef}
        groupId={currentGroup}
        projectId={currentProject}
        versionId={currentVersion}
        scanId={currentScan}
        prescanLoading={prescanLoading}
        selectedConfig={selectedConfig}
        onSelectConfig={(data) => setSelectedConfig(data)}
        onCloseConfig={() => setSelectedConfig(null)}
        results={results}
        onAddConfig={handleAddConfig}
        configPathBeingEdited={configPathBeingEdited}
        editingConfig={editingConfig}
        onSaveConfig={handleSaveConfig}
        onCancelEditConfig={handleCancelEditConfig}
      />

    </div>
  );
};
