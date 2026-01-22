import React, { useState, useEffect, useRef, useLayoutEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import { DiffViewer } from './DiffViewer';
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

export const ScanResults = () => {
  const { currentGroup, currentProject, currentVersion, currentScan, scanInfo, loading, error, loadScanInfo } = useScanner();
  const [expandedResultId, setExpandedResultId] = useState(null);
  const scrollContainerRef = useRef(null);
  const scrollPositionRef = useRef(0);
  const previousResultLengthRef = useRef(0);

  // Load scan info automatically when a scan is selected
  useEffect(() => {
    if (currentGroup != null && currentProject != null && currentVersion != null && currentScan != null) {
      loadScanInfo(currentGroup, currentProject, currentVersion, currentScan);
    }
  }, [currentGroup, currentProject, currentVersion, currentScan, loadScanInfo]);

  // Save scroll position before polling updates
  useEffect(() => {
    if (!scrollContainerRef.current) return;

    const handleScroll = () => {
      if (scrollContainerRef.current) {
        scrollPositionRef.current = scrollContainerRef.current.scrollTop;
      }
    };

    const container = scrollContainerRef.current;
    container.addEventListener('scroll', handleScroll);
    return () => container.removeEventListener('scroll', handleScroll);
  }, []);

  // Restore scroll position after updates using useLayoutEffect for synchronous restoration
  useLayoutEffect(() => {
    if (scrollContainerRef.current && scanInfo?.results) {
      // Only reset scroll if results actually changed (new items added), not during ongoing updates
      const currentResultLength = scanInfo.results.length;
      if (currentResultLength !== previousResultLengthRef.current) {
        previousResultLengthRef.current = currentResultLength;
        // If new results were added, restore the previous scroll position
        scrollContainerRef.current.scrollTop = scrollPositionRef.current;
      }
    }
  }, [scanInfo]);

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

  const { results = [], status, finishedCount, maxCount } = scanInfo;
  const progress = maxCount > 0 ? (finishedCount / maxCount) * 100 : 0;

  const toggleExpanded = (id) => {
    setExpandedResultId(expandedResultId === id ? null : id);
  };

  const getSeverityColor = (matchCount) => {
    if (matchCount === 0) return 'green';
    if (matchCount < 5) return 'yellow';
    if (matchCount < 20) return 'orange';
    return 'red';
  };

  return (
    <div className="component-container">
      <h3>Scan Results</h3>
      {error && <div className="error-message">{error}</div>}

      {status && (
        <div className="scan-header">
          <span className={`status-badge status-${getStatusName(status)}`}>{getStatusName(status).toUpperCase()}</span>
          <span className="result-count">{results.length} matches found</span>
        </div>
      )}

      {/* Progress Bar */}
      {maxCount > 0 && (
        <div className="progress-container">
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

      {results.length === 0 ? (
        <p className="empty-message">No matches found in this scan</p>
      ) : (
        <div className="results-container" ref={scrollContainerRef}>
          {results.map((result, idx) => (
            <div
              key={idx}
              className="result-item"
              onClick={() => toggleExpanded(idx)}
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
  );
};
