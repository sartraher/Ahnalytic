import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/components.css';

export const GroupsList = () => {
  const { groups, currentGroup, setCurrentGroup, loadGroups, deleteExistingGroup, loading, error } = useScanner();
  const [expandedGroupId, setExpandedGroupId] = useState(null);

  useEffect(() => {
    loadGroups();
  }, [loadGroups]);

  const handleDelete = async (id) => {
    if (window.confirm('Are you sure you want to delete this group?')) {
      try {
        await deleteExistingGroup(id);
      } catch (err) {
        console.error('Delete failed:', err);
      }
    }
  };

  const toggleExpanded = (id) => {
    setExpandedGroupId(expandedGroupId === id ? null : id);
  };

  if (loading && Object.keys(groups).length === 0) {
    return <div className="loading">Loading groups...</div>;
  }

  return (
    <div className="component-container">
      <h3>Groups</h3>
      {error && <div className="error-message">{error}</div>}
      
      {Object.keys(groups).length === 0 ? (
        <p className="empty-message">No groups yet. Create one to get started!</p>
      ) : (
        <div className="list-container">
          {Object.entries(groups).map(([id, name]) => (
            <div
              key={id}
              className={`list-item ${currentGroup === parseInt(id) ? 'active' : ''}`}
              onClick={() => setCurrentGroup(parseInt(id))}
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
              {expandedGroupId === id && (
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

export const GroupForm = () => {
  const { createNewGroup, loading, error } = useScanner();
  const [name, setName] = useState('');
  const [submitting, setSubmitting] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!name.trim()) return;

    try {
      setSubmitting(true);
      await createNewGroup(name);
    } catch (err) {
      console.error('Create failed:', err);
    } finally {
      setSubmitting(false);
      setName('');
    }
  };

  return (
    <form className="form-container" onSubmit={handleSubmit}>
      <h4>Create New Group</h4>
      {error && <div className="error-message">{error}</div>}
      
      <div className="form-group">
        <label htmlFor="group-name">Group Name</label>
        <input
          id="group-name"
          type="text"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="Enter group name"
          disabled={loading || submitting}
        />
      </div>

      <button
        type="submit"
        className="btn-primary"
        disabled={loading || submitting || !name.trim()}
      >
        {submitting ? 'Creating...' : 'Create Group'}
      </button>
    </form>
  );
};
