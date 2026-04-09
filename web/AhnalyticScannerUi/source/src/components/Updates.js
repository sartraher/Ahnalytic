import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/components.css';

export const UpdatesList = () => {
  const {
    updates,
    updateStatus,
    updateQueuedWaiting,
    checkForUpdates,
    loading,
    error,
  } = useScanner();

  const [currentPage, setCurrentPage] = useState(1);
  const itemsPerPage = 50;

  useEffect(() => {
    // Check for updates on component mount
    checkForUpdates();
    
    // Poll for updates every 30 seconds
    const interval = setInterval(() => {
      checkForUpdates();
    }, 30000);

    return () => clearInterval(interval);
  }, [checkForUpdates]);

  // Calculate pagination
  const totalPages = Math.ceil(updates.length / itemsPerPage);
  const startIndex = (currentPage - 1) * itemsPerPage;
  const endIndex = startIndex + itemsPerPage;
  const paginatedUpdates = updates.slice(startIndex, endIndex);

  // Reset to page 1 if current page exceeds max pages
  useEffect(() => {
    if (currentPage > totalPages && totalPages > 0) {
      setCurrentPage(1);
    }
  }, [updates.length, currentPage, totalPages]);

  if (loading && updates.length === 0) {
    return <div className="loading">Loading updates...</div>;
  }

  return (
    <div className="component-container">
      <div className="list-header">
        <h3>Available Updates</h3>
        <span className="update-count">({updates.length} total)</span>
      </div>
      {error && <div className="error-message">{error}</div>}
      {updateQueuedWaiting && (
        <div className="warning-message">
          Updates are queued and waiting for the current scan to complete...
        </div>
      )}

      {updates.length === 0 ? (
        <p className="empty-message">No updates available</p>
      ) : (
        <>
          <div className="list-container compact">
            {paginatedUpdates.map((update, index) => (
              <div key={startIndex + index} className="list-item-compact">
                <span className="update-name">{update.name}</span>
                <span className="update-type-badge">{update.type}</span>
                <span className="update-language">{update.language}</span>
                <span className="update-sha" title={update.sha}>{update.sha?.substring(0, 8)}</span>
              </div>
            ))}
          </div>

          {/* Pagination Controls */}
          {totalPages > 1 && (
            <div className="pagination-container">
              <button
                className="btn-pagination"
                onClick={() => setCurrentPage(Math.max(1, currentPage - 1))}
                disabled={currentPage === 1}
              >
                ← Previous
              </button>
              <div className="pagination-info">
                Page {currentPage} of {totalPages}
              </div>
              <button
                className="btn-pagination"
                onClick={() => setCurrentPage(Math.min(totalPages, currentPage + 1))}
                disabled={currentPage === totalPages}
              >
                Next →
              </button>
            </div>
          )}
        </>
      )}
    </div>
  );
};

export const UpdateControls = () => {
  const {
    updateStatus,
    loading,
    updateQueuedWaiting,
    startExistingUpdate,
    fetchUpdateStatus,
  } = useScanner();
  const [polling, setPolling] = useState(true); // Start polling immediately

  // Fetch update status on component mount to get fresh data from backend
  useEffect(() => {
    fetchUpdateStatus();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Auto-poll for update status continuously
  useEffect(() => {
    if (!polling) return;

    // Fetch immediately
    fetchUpdateStatus();

    const interval = setInterval(() => {
      fetchUpdateStatus();
    }, 2000); // Poll every 2 seconds

    return () => clearInterval(interval);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [polling]);

  // Stop polling only if update is complete AND we manually stopped it
  const handleStart = async () => {
    try {
      await startExistingUpdate();
      // Keep polling active
    } catch (err) {
      console.error('Start update failed:', err);
    }
  };

  return (
    <div className="form-container">
      <h4>Update Controls</h4>
      {updateStatus && (
        <>
          <div className="update-info">
            <div className="info-row">
              <strong>Status:</strong>
              <span className={`status-badge status-${updateStatus.inUpdate ? 'running' : 'idle'}`}>
                {updateStatus.inUpdate ? 'UPDATING' : 'IDLE'}
              </span>
            </div>
          </div>

          {/* Progress Bar */}
          {updateStatus.amountMax > 0 && (
            <div className="progress-container">
              <div className="progress-label">Update Progress</div>
              <div className="progress-bar-wrapper">
                <div className="progress-bar">
                  <div 
                    className="progress-fill" 
                    style={{ width: `${(updateStatus.amountFinished / updateStatus.amountMax) * 100}%` }}
                  ></div>
                </div>
              </div>
              <span className="progress-text">
                {updateStatus.amountFinished} / {updateStatus.amountMax}
              </span>
            </div>
          )}
        </>
      )}

      {updateQueuedWaiting && (
        <div className="queued-message">
          <p>Update is queued and waiting for scan to complete...</p>
        </div>
      )}

      <div className="button-group">
        <button
          onClick={handleStart}
          disabled={loading || updateStatus?.inUpdate || updateQueuedWaiting}
          className="btn-success"
        >
          {updateStatus?.inUpdate ? 'Updating...' : 'Start Update'}
        </button>
      </div>
    </div>
  );
};
