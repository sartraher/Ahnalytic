import React, { useState } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/components.css';

export const ScanResults = () => {
  const { currentGroup, currentProject, currentVersion, currentScan, scanInfo, loading, error } = useScanner();
  const [expandedResultId, setExpandedResultId] = useState(null);

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
          <span className={`status-badge status-${status}`}>{status.toUpperCase()}</span>
          <span className="result-count">{results.length} matches found</span>
        </div>
      )}

      {results.length === 0 ? (
        <p className="empty-message">No matches found in this scan</p>
      ) : (
        <div className="results-container">
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
