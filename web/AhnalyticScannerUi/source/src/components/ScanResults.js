import React, { useState, useEffect, useRef, useMemo } from 'react';
import { useScanner } from '../context/ScannerContext';
import { DiffViewer } from './DiffViewer';
import api from '../services/api';
import '../styles/components.css';

// Helper function to convert numeric status codes to status names
const getStatusName = (status) => {
  if (typeof status === 'string') return status;
  const statusMap = {
    0: 'pending',
    1: 'started',
    2: 'running',
    3: 'completed',
    4: 'aborted',
    5: 'failed',
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

// File tree renderer - properly shows folders and files in tree structure
const FileTreeRenderer = ({ tree, selectedPath, onSelect, expandedPaths, onToggleExpand }) => {
  const renderNode = (node) => {
    const isSelected = selectedPath === node.path;
    const isExpanded = expandedPaths[node.path] || false;
    const hasChildren = Object.keys(node.children).length > 0;
    const isFolder = !node.isFile;

    return (
      <div key={node.path} className="file-tree-node">
        <div
          className={`file-tree-node-content ${isSelected ? 'active' : ''} ${isFolder ? 'folder' : 'file'}`}
          onClick={(e) => {
            e.stopPropagation();
            // Toggle expand for folders
            if (isFolder && hasChildren) {
              onToggleExpand(node.path);
            } else {
              // Only select files or empty folders
              onSelect(node.path);
            }
          }}
        >
          {isFolder ? (
            <>
              {hasChildren ? (
                <span className="file-tree-expander">
                  {isExpanded ? '▼' : '▶'}
                </span>
              ) : (
                <span className="file-tree-expander empty"></span>
              )}
              <span className="file-tree-icon">📁</span>
            </>
          ) : (
            <>
              <span className="file-tree-expander empty"></span>
              <span className="file-tree-icon">📄</span>
            </>
          )}
          <span className="file-tree-label">{node.name}</span>
          {isFolder && hasChildren && (
            <span className="file-tree-count">({Object.values(node.children).flat().length})</span>
          )}
          {!isFolder && node.resultCount > 0 && (
            <span className="file-result-count">{node.resultCount}</span>
          )}
        </div>
        {isExpanded && isFolder && hasChildren && (
          <div className="file-tree-children">
            {Object.values(node.children).map((child) => renderNode(child))}
          </div>
        )}
      </div>
    );
  };

  return (
    <div className="file-tree-container">
      {Object.values(tree).map((node) => renderNode(node))}
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
          <div className="progress-label">Files Scanned</div>
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
  scrollContainerRef 
}) => {
  const getSeverityColor = (matchCount) => {
    if (matchCount === 0) return 'green';
    if (matchCount < 5) return 'yellow';
    if (matchCount < 20) return 'orange';
    return 'red';
  };

  return (
    <div className="scan-results-content">
      {/* File Tree Panel */}
      <div className="file-tree-panel">
        <div className="file-tree-header">
          <h4>Files</h4>
          {selectedFilePath && (
            <button
              className="btn-icon"
              onClick={() => onSelectFile(null)}
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
        />
      </div>

      {/* Results Panel */}
      <div className="results-panel">
        {filteredResults.length === 0 ? (
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
                        
                        {/* Show Diff Viewer if content is available */}
                        {result.searchContent && result.sourceContent && (
                          <div className="detail-subsection">
                            <DiffViewer
                              searchContent={result.searchContent}
                              sourceContent={result.sourceContent}
                              resultSets={result.resultSets}
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
  const scrollContainerRef = useRef(null);
  const scrollPositionRef = useRef(0);
  
  // Separate progress state to avoid re-rendering the entire component during polling
  const [progress, setProgress] = useState({ finishedCount: 0, maxCount: 0, deepFinishedCount: 0, deepMaxCount: 0 });
  const progressTimeoutRef = useRef(null);

  // Load scan info automatically when a scan is selected
  useEffect(() => {
    if (currentGroup != null && currentProject != null && currentVersion != null && currentScan != null) {
      loadScanInfo(currentGroup, currentProject, currentVersion, currentScan);
    }
  }, [currentGroup, currentProject, currentVersion, currentScan, loadScanInfo]);

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

    // Check if scan is still running based on current scanInfo
    const isRunning = scanInfo && (scanInfo.status === 1 || scanInfo.status === 2 || scanInfo.status === 'started' || scanInfo.status === 'running');
    
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

  // Build file tree from results
  const fileTree = useMemo(() => {
    if (!scanInfo || !scanInfo.results) return {};
    return buildFileTree(scanInfo.results);
  }, [scanInfo]);

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

      {results.length === 0 ? (
        <p className="empty-message">No matches found in this scan</p>
      ) : (
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
        />
      )}
    </div>
  );
};
