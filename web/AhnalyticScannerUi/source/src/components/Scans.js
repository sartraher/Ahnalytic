import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
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

export const ScansList = () => {
  const {
    currentGroup,
    currentProject,
    currentVersion,
    scans,
    currentScan,
    setCurrentScan,
    loadScans,
    loading,
    error,
  } = useScanner();
  const [expandedScanId, setExpandedScanId] = useState(null);

  useEffect(() => {
    if (currentGroup != null && currentProject != null && currentVersion != null) {
      loadScans(currentGroup, currentProject, currentVersion);
    }
  }, [currentGroup, currentProject, currentVersion, loadScans]);

  if (currentGroup == null || currentProject == null || currentVersion == null) {
    return <div className="component-container"><p className="empty-message">Select a group, project, and version first</p></div>;
  }

  const toggleExpanded = (id) => {
    setExpandedScanId(expandedScanId === id ? null : id);
  };

  if (loading && Object.keys(scans).length === 0) {
    return <div className="loading">Loading scans...</div>;
  }

  return (
    <div className="component-container">
      <h3>Scans</h3>
      {error && <div className="error-message">{error}</div>}

      {Object.keys(scans).length === 0 ? (
        <p className="empty-message">No scans yet. Create one!</p>
      ) : (
        <div className="list-container">
          {Object.entries(scans).map(([id, name]) => (
            <div
              key={id}
              className={`list-item ${currentScan === parseInt(id) ? 'active' : ''}`}
              onClick={() => setCurrentScan(parseInt(id))}
            >
              <div className="list-item-header">
                <span className="list-item-title">{name}</span>
                <div className="list-item-actions">
                  <button
                    className="btn-icon"
                    onClick={(e) => {
                      e.stopPropagation();
                      toggleExpanded(id);
                    }}
                    title="Show options"
                  >
                    ⋮
                  </button>
                </div>
              </div>
              {expandedScanId === id && (
                <div className="list-item-menu">
                  <button className="btn-info">View Details</button>
                </div>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

export const ScanForm = () => {
  const { currentGroup, currentProject, currentVersion, createNewScan, loading, error } = useScanner();
  const [name, setName] = useState('');
  const [submitting, setSubmitting] = useState(false);

  if (currentGroup == null || currentProject == null || currentVersion == null) {
    return null;
  }

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!name.trim()) return;

    try {
      setSubmitting(true);
      await createNewScan(currentGroup, currentProject, currentVersion, name);
    } catch (err) {
      console.error('Create failed:', err);
    } finally {
      setSubmitting(false);
      setName('');
    }
  };

  return (
    <form className="form-container" onSubmit={handleSubmit}>
      <h4>Create New Scan</h4>
      {error && <div className="error-message">{error}</div>}

      <div className="form-group">
        <label htmlFor="scan-name">Scan Name</label>
        <input
          id="scan-name"
          type="text"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="e.g., Initial Scan"
          disabled={loading || submitting}
        />
      </div>

      <button
        type="submit"
        className="btn-primary"
        disabled={loading || submitting || !name.trim()}
      >
        {submitting ? 'Creating...' : 'Create Scan'}
      </button>
    </form>
  );
};

export const ScanFileUpload = () => {
  const {
    currentGroup,
    currentProject,
    currentVersion,
    currentScan,
    uploadFile,
    loading,
    error,
  } = useScanner();
  const [uploading, setUploading] = useState(false);
  const [uploadError, setUploadError] = useState(null);
  const fileInputRef = React.useRef(null);

  if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
    return null;
  }

  const handleFileChange = async (e) => {
    const file = e.target.files?.[0];
    if (!file) return;

    // Validate file type
    const validTypes = ['application/zip', 'application/gzip', 'application/x-gzip', 'application/x-tar'];
    if (!validTypes.some(type => file.type === type || file.name.endsWith('.zip') || file.name.endsWith('.tar.gz'))) {
      setUploadError('Please upload a .zip or .tar.gz file');
      return;
    }

    try {
      setUploading(true);
      setUploadError(null);
      await uploadFile(currentGroup, currentProject, currentVersion, currentScan, file);
      if (fileInputRef.current) {
        fileInputRef.current.value = '';
      }
    } catch (err) {
      setUploadError(err.message || 'Upload failed');
    } finally {
      setUploading(false);
    }
  };

  return (
    <div className="form-container">
      <h4>Upload Code Archive</h4>
      {uploadError && <div className="error-message">{uploadError}</div>}
      {error && <div className="error-message">{error}</div>}

      <div className="form-group">
        <label htmlFor="file-upload">Select File (.zip or .tar.gz)</label>
        <input
          ref={fileInputRef}
          id="file-upload"
          type="file"
          accept=".zip,.tar.gz,.tgz"
          onChange={handleFileChange}
          disabled={loading || uploading}
        />
        <small>Accepted formats: ZIP, TAR.GZ</small>
      </div>

      {uploading && <div className="loading">Uploading...</div>}
    </div>
  );
};

export const ScanControls = () => {
  const {
    currentGroup,
    currentProject,
    currentVersion,
    currentScan,
    scanInfo,
    loading,
    startExistingScan,
    abortExistingScan,
    loadScanInfo,
  } = useScanner();
  const [polling, setPolling] = useState(false);

  // Auto-poll for scan info while scanning
  useEffect(() => {
    if (!polling || !currentGroup || !currentProject || !currentVersion || !currentScan) return;

    const interval = setInterval(() => {
      loadScanInfo(currentGroup, currentProject, currentVersion, currentScan);
    }, 3000); // Poll every 3 seconds

    return () => clearInterval(interval);
  }, [polling, currentGroup, currentProject, currentVersion, currentScan, loadScanInfo]);

  // Stop polling if scan is complete
  useEffect(() => {
    const statusName = getStatusName(scanInfo?.status);
    if (statusName === 'completed' || statusName === 'aborted' || statusName === 'failed') {
      setPolling(false);
    }
  }, [scanInfo?.status]);

  if (currentGroup == null || currentProject == null || currentVersion == null || currentScan == null) {
    return null;
  }

  const handleStart = async () => {
    try {
      await startExistingScan(currentGroup, currentProject, currentVersion, currentScan);
      setPolling(true);
    } catch (err) {
      console.error('Start failed:', err);
    }
  };

  const handleAbort = async () => {
    if (window.confirm('Are you sure you want to abort this scan?')) {
      try {
        await abortExistingScan(currentGroup, currentProject, currentVersion, currentScan);
        setPolling(false);
      } catch (err) {
        console.error('Abort failed:', err);
      }
    }
  };

  const statusName = getStatusName(scanInfo?.status);
  const isRunning = statusName === 'running' || statusName === 'started';

  return (
    <div className="form-container">
      <h4>Scan Controls</h4>
      {scanInfo && (
        <>
          <div className="scan-info">
            <div className="info-row">
              <strong>Status:</strong>
              <span className={`status-badge status-${getStatusName(scanInfo.status)}`}>
                {getStatusName(scanInfo.status).toUpperCase()}
              </span>
            </div>
            {scanInfo.results && (
              <div className="info-row">
                <strong>Matches Found:</strong>
                <span>{scanInfo.results.length}</span>
              </div>
            )}
          </div>
        </>
      )}

      <div className="button-group">
        <button
          onClick={handleStart}
          disabled={loading || isRunning}
          className="btn-success"
        >
          {isRunning ? 'Scanning...' : 'Start Scan'}
        </button>
        {isRunning && (
          <button
            onClick={handleAbort}
            disabled={loading}
            className="btn-danger"
          >
            Abort Scan
          </button>
        )}
      </div>
    </div>
  );
};
