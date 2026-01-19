import React, { useState } from 'react';
import '../styles/diffviewer.css';

export const DiffViewer = ({ searchContent, sourceContent, resultSets }) => {
  const [selectedMatch, setSelectedMatch] = useState(0);

  if (!resultSets || resultSets.length === 0) {
    return <div className="diff-viewer-empty">No matches to display</div>;
  }

  if (!searchContent || !sourceContent) {
    return <div className="diff-viewer-empty">Content not available</div>;
  }

  const splitContent = (content, startLine, endLine) => {
    const lines = content.split('\n');
    // Lines are 1-indexed in the API, but array is 0-indexed
    const start = Math.max(0, startLine - 1);
    const end = Math.min(lines.length, endLine);
    return lines.slice(start, end);
  };

  const match = resultSets[selectedMatch];
  const sourceLines = splitContent(sourceContent, match.baseStart, match.baseEnd);
  const searchLines = splitContent(searchContent, match.searchStart, match.searchEnd);

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
