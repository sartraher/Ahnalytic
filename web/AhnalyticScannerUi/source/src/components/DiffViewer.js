import React, { useState, useEffect } from 'react';
import api from '../services/api';
import '../styles/diffviewer.css';

// Utility function to decode base64 content
const decodeBase64 = (encoded) => {
  if (!encoded) return '';
  try {
    return atob(encoded);
  } catch (e) {
    console.error('Failed to decode base64:', e);
    return encoded; // Return original if decoding fails
  }
};

export const DiffViewer = ({ 
  elementIndex, 
  resultSets,
  groupId,
  projectId,
  versionId,
  scanId
}) => {
  const [selectedMatch, setSelectedMatch] = useState(0);
  const [searchContent, setSearchContent] = useState(null);
  const [sourceContent, setSourceContent] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  // Load file content on demand
  useEffect(() => {
    if (elementIndex === undefined || groupId === null || projectId === null || versionId === null || scanId === null) {
      return;
    }

    const loadFileContent = async () => {
      setLoading(true);
      setError(null);
      try {
        const data = await api.getFileContent(groupId, projectId, versionId, scanId, elementIndex);
        if (data) {
          setSearchContent(data.searchContent);
          setSourceContent(data.sourceContent);
        }
      } catch (err) {
        console.error('Failed to load file content:', err);
        setError('Failed to load file content');
      } finally {
        setLoading(false);
      }
    };

    loadFileContent();
  }, [elementIndex, groupId, projectId, versionId, scanId]);

  if (!resultSets || resultSets.length === 0) {
    return <div className="diff-viewer-empty">No matches to display</div>;
  }

  if (loading) {
    return <div className="diff-viewer-empty">Loading file content...</div>;
  }

  if (error) {
    return <div className="diff-viewer-empty error">{error}</div>;
  }

  // Check if content was successfully loaded
  if (searchContent === null || sourceContent === null) {
    return <div className="diff-viewer-empty">No content loaded</div>;
  }

  // Decode base64 content
  const decodedSearchContent = decodeBase64(searchContent);
  const decodedSourceContent = decodeBase64(sourceContent);

  const splitContent = (content, startLine, endLine) => {
    const lines = content.split('\n');
    // Lines are 1-indexed in the API, but array is 0-indexed
    const start = Math.max(0, startLine - 1);
    const end = Math.min(lines.length, endLine);
    return lines.slice(start, end);
  };

  const match = resultSets[selectedMatch];
  const sourceLines = splitContent(decodedSourceContent, match.baseStart, match.baseEnd);
  const searchLines = splitContent(decodedSearchContent, match.searchStart, match.searchEnd);

  return (
    <div className="diff-viewer">
      {/* Match Selector */}
      {resultSets.length > 1 && (
        <div className="diff-match-selector">
          <span className="selector-label">
            Match {selectedMatch + 1} of {resultSets.length}
          </span>
          <div className="selector-buttons">
            <button
              className="btn-selector"
              onClick={() => setSelectedMatch(Math.max(0, selectedMatch - 1))}
              disabled={selectedMatch === 0}
            >
              ← Previous
            </button>
            <button
              className="btn-selector"
              onClick={() => setSelectedMatch(Math.min(resultSets.length - 1, selectedMatch + 1))}
              disabled={selectedMatch === resultSets.length - 1}
            >
              Next →
            </button>
          </div>
        </div>
      )}

      {/* Diff Container */}
      <div className="diff-container">
        {/* Source File Column */}
        <div className="diff-column diff-source">
          <div className="diff-header">
            <span className="diff-header-title">Source File</span>
            <span className="diff-line-range">
              Lines {match.baseStart}-{match.baseEnd}
            </span>
          </div>
          <div className="diff-content">
            <table className="diff-table">
              <tbody>
                {sourceLines.map((line, idx) => (
                  <tr key={`source-${idx}`} className="diff-line">
                    <td className="diff-line-number">{match.baseStart + idx}</td>
                    <td className="diff-line-content">
                      <code>{line || '\u00A0'}</code>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        {/* Search File Column */}
        <div className="diff-column diff-search">
          <div className="diff-header">
            <span className="diff-header-title">Search File</span>
            <span className="diff-line-range">
              Lines {match.searchStart}-{match.searchEnd}
            </span>
          </div>
          <div className="diff-content">
            <table className="diff-table">
              <tbody>
                {searchLines.map((line, idx) => (
                  <tr key={`search-${idx}`} className="diff-line">
                    <td className="diff-line-number">{match.searchStart + idx}</td>
                    <td className="diff-line-content">
                      <code>{line || '\u00A0'}</code>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
};
