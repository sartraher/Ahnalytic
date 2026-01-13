import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/components.css';

export const ProjectsList = () => {
  const {
    currentGroup,
    projects,
    currentProject,
    setCurrentProject,
    loadProjects,
    deleteExistingProject,
    loading,
    error,
  } = useScanner();
  const [expandedProjectId, setExpandedProjectId] = useState(null);

  useEffect(() => {
    if (currentGroup != null) {
      loadProjects(currentGroup);
    }
  }, [currentGroup, loadProjects]);

  if (currentGroup == null) {
    return <div className="component-container"><p className="empty-message">Select a group first</p></div>;
  }

  const handleDelete = async (id) => {
    if (window.confirm('Are you sure you want to delete this project?')) {
      try {
        await deleteExistingProject(currentGroup, id);
      } catch (err) {
        console.error('Delete failed:', err);
      }
    }
  };

  const toggleExpanded = (id) => {
    setExpandedProjectId(expandedProjectId === id ? null : id);
  };

  if (loading && Object.keys(projects).length === 0) {
    return <div className="loading">Loading projects...</div>;
  }

  return (
    <div className="component-container">
      <h3>Projects</h3>
      {error && <div className="error-message">{error}</div>}

      {Object.keys(projects).length === 0 ? (
        <p className="empty-message">No projects yet. Create one!</p>
      ) : (
        <div className="list-container">
          {Object.entries(projects).map(([id, name]) => (
            <div
              key={id}
              className={`list-item ${currentProject === parseInt(id) ? 'active' : ''}`}
              onClick={() => setCurrentProject(parseInt(id))}
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
              {expandedProjectId === id && (
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

export const ProjectForm = () => {
  const { currentGroup, createNewProject, loading, error } = useScanner();
  const [name, setName] = useState('');
  const [submitting, setSubmitting] = useState(false);

  if (currentGroup == null) {
    return null;
  }

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!name.trim()) return;

    try {
      setSubmitting(true);
      await createNewProject(currentGroup, name);
    } catch (err) {
      console.error('Create failed:', err);
    } finally {
      setSubmitting(false);
      setName('');
    }
  };

  return (
    <form className="form-container" onSubmit={handleSubmit}>
      <h4>Create New Project</h4>
      {error && <div className="error-message">{error}</div>}

      <div className="form-group">
        <label htmlFor="project-name">Project Name</label>
        <input
          id="project-name"
          type="text"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="Enter project name"
          disabled={loading || submitting}
        />
      </div>

      <button
        type="submit"
        className="btn-primary"
        disabled={loading || submitting || !name.trim()}
      >
        {submitting ? 'Creating...' : 'Create Project'}
      </button>
    </form>
  );
};
