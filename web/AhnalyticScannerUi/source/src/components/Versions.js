import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/components.css';

export const VersionsList = () => {
  const {
    currentGroup,
    currentProject,
    versions,
    currentVersion,
    setCurrentVersion,
    loadVersions,
    deleteExistingVersion,
    loading,
    error,
  } = useScanner();
  const [expandedVersionId, setExpandedVersionId] = useState(null);

  useEffect(() => {
    if (currentGroup != null && currentProject != null) {
      loadVersions(currentGroup, currentProject);
    }
  }, [currentGroup, currentProject, loadVersions]);

  if (currentGroup == null || currentProject == null) {
    return <div className="component-container"><p className="empty-message">Select a group and project first</p></div>;
  }

  const handleDelete = async (id) => {
    if (window.confirm('Are you sure you want to delete this version?')) {
      try {
        await deleteExistingVersion(currentGroup, currentProject, id);
      } catch (err) {
        console.error('Delete failed:', err);
      }
    }
  };

  const toggleExpanded = (id) => {
    setExpandedVersionId(expandedVersionId === id ? null : id);
  };

  if (loading && Object.keys(versions).length === 0) {
    return <div className="loading">Loading versions...</div>;
  }

  return (
    <div className="component-container">
      <h3>Versions</h3>
      {error && <div className="error-message">{error}</div>}

      {Object.keys(versions).length === 0 ? (
        <p className="empty-message">No versions yet. Create one!</p>
      ) : (
        <div className="list-container">
          {Object.entries(versions).map(([id, name]) => (
            <div
              key={id}
              className={`list-item ${currentVersion === parseInt(id) ? 'active' : ''}`}
              onClick={() => setCurrentVersion(parseInt(id))}
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
              {expandedVersionId === id && (
                <div className="list-item-menu">
                  <button
                    className="btn-danger"
                    onClick={(e) => {
                      e.stopPropagation();
                      handleDelete(id);
                    }}
                  >
                    Delete
                  </button>
                </div>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

export const VersionForm = () => {
  const { currentGroup, currentProject, createNewVersion, loading, error } = useScanner();
  const [name, setName] = useState('');
  const [submitting, setSubmitting] = useState(false);

  if (currentGroup == null || currentProject == null) {
    return null;
  }

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!name.trim()) return;

    try {
      setSubmitting(true);
      await createNewVersion(currentGroup, currentProject, name);
    } catch (err) {
      console.error('Create failed:', err);
    } finally {
      setSubmitting(false);
      setName('');
    }
  };

  return (
    <form className="form-container" onSubmit={handleSubmit}>
      <h4>Create New Version</h4>
      {error && <div className="error-message">{error}</div>}

      <div className="form-group">
        <label htmlFor="version-name">Version Name</label>
        <input
          id="version-name"
          type="text"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="e.g., 1.0.0"
          disabled={loading || submitting}
        />
      </div>

      <button
        type="submit"
        className="btn-primary"
        disabled={loading || submitting || !name.trim()}
      >
        {submitting ? 'Creating...' : 'Create Version'}
      </button>
    </form>
  );
};
