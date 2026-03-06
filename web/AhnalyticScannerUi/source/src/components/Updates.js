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

  useEffect(() => {
    // Check for updates on component mount
    checkForUpdates();
    
    // Poll for updates every 30 seconds
    const interval = setInterval(() => {
      checkForUpdates();
    }, 30000);

    return () => clearInterval(interval);
  }, [checkForUpdates]);

  if (loading && updates.length === 0) {
    return <div className="loading">Loading updates...</div>;
  }

  return (
    <div className="component-container">
      <h3>Available Updates</h3>
      {error && <div className="error-message">{error}</div>}
      {updateQueuedWaiting && (
        <div className="warning-message">
          Updates are queued and waiting for the current scan to complete...
        </div>
      )}

      {updates.length === 0 ? (
        <p className="empty-message">No updates available</p>
      ) : (
        <div className="list-container">
          {updates.map((update, index) => (
            <div key={index} className="list-item">
              <div className="list-item-header">
                <span className="list-item-title">{update.name}</span>
                <span className="update-badge">{update.type}</span>
              </div>
              <div className="update-details">
                <div className="detail-row">
                  <strong>Language:</strong> {update.language}
                </div>
                <div className="detail-row">
                  <strong>SHA:</strong> {update.sha}
                </div>
              </div>
            </div>
          ))}
        </div>
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
  const [polling, setPolling] = useState(false);

  // Auto-poll for update status while updating
  useEffect(() => {
    if (!polling) return;

    const interval = setInterval(() => {
      fetchUpdateStatus();
    }, 2000); // Poll every 2 seconds

    return () => clearInterval(interval);
  }, [polling, fetchUpdateStatus]);

  // Stop polling if update is complete
  useEffect(() => {
    if (updateStatus?.inUpdate === false) {
      setPolling(false);
    }
  }, [updateStatus?.inUpdate]);

  const handleStart = async () => {
    try {
      await startExistingUpdate();
      setPolling(true);
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
