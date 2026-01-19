import React, { useState, useEffect } from 'react';
import { useScanner } from '../context/ScannerContext';
import '../styles/treeview.css';

export const TreeView = () => {
  const {
    groups,
    projects,
    versions,
    scans,
    currentGroup,
    currentProject,
    currentVersion,
    currentScan,
    setCurrentGroup,
    setCurrentProject,
    setCurrentVersion,
    setCurrentScan,
    createNewGroup,
    createNewProject,
    createNewVersion,
    createNewScan,
    deleteExistingGroup,
    deleteExistingProject,
    deleteExistingVersion,
    loadGroups,
    loadProjects,
    loadVersions,
    loadScans,
    loading,
  } = useScanner();

  const [expandedGroups, setExpandedGroups] = useState({});
  const [expandedProjects, setExpandedProjects] = useState({});
  const [expandedVersions, setExpandedVersions] = useState({});
  const [newGroupName, setNewGroupName] = useState('');
  const [newProjectName, setNewProjectName] = useState('');
  const [newVersionName, setNewVersionName] = useState('');
  const [newScanName, setNewScanName] = useState('');
  const [showGroupForm, setShowGroupForm] = useState(false);
  const [showProjectForm, setShowProjectForm] = useState(false);
  const [showVersionForm, setShowVersionForm] = useState(false);
  const [showScanForm, setShowScanForm] = useState(false);

  // Load groups on mount
  useEffect(() => {
    loadGroups();
  }, [loadGroups]);

  // Auto-expand groups when a new one is created
  useEffect(() => {
    if (currentGroup != null) {
      setExpandedGroups((prev) => ({
        ...prev,
        [currentGroup]: true,
      }));
    }
  }, [currentGroup, groups]);

  // Auto-expand projects and their parent group when a new one is created
  useEffect(() => {
    if (currentProject != null && currentGroup != null) {
      setExpandedProjects((prev) => ({
        ...prev,
        [currentProject]: true,
      }));
      setExpandedGroups((prev) => ({
        ...prev,
        [currentGroup]: true,
      }));
    }
  }, [currentProject, currentGroup, projects]);

  // Auto-expand versions and their parent project when a new one is created
  useEffect(() => {
    if (currentVersion != null && currentProject != null) {
      setExpandedVersions((prev) => ({
        ...prev,
        [currentVersion]: true,
      }));
      setExpandedProjects((prev) => ({
        ...prev,
        [currentProject]: true,
      }));
    }
  }, [currentVersion, currentProject, versions]);

  // Auto-select and ensure parent version is expanded when a scan is created
  useEffect(() => {
    if (currentVersion != null && Object.keys(scans).length > 0 && currentScan == null) {
      const firstScanId = Object.keys(scans)[0];
      setCurrentScan(parseInt(firstScanId));
      setExpandedVersions((prev) => ({
        ...prev,
        [currentVersion]: true,
      }));
    }
  }, [scans, currentVersion, currentScan, setCurrentScan]);

  const toggleGroup = (groupId) => {
    setExpandedGroups((prev) => ({
      ...prev,
      [groupId]: !prev[groupId],
    }));
  };

  const handleSelectGroup = async (groupId) => {
    setCurrentGroup(parseInt(groupId));
    setExpandedGroups((prev) => ({
      ...prev,
      [groupId]: !prev[groupId],
    }));
    // Load projects for this group
    await loadProjects(parseInt(groupId));
  };

  const handleSelectProject = async (projectId, groupId) => {
    setCurrentProject(parseInt(projectId));
    setExpandedProjects((prev) => ({
      ...prev,
      [projectId]: !prev[projectId],
    }));
    // Load versions for this project
    await loadVersions(parseInt(groupId), parseInt(projectId));
  };

  const handleSelectVersion = async (versionId, groupId, projectId) => {
    setCurrentVersion(parseInt(versionId));
    setExpandedVersions((prev) => ({
      ...prev,
      [versionId]: !prev[versionId],
    }));
    // Load scans for this version
    await loadScans(parseInt(groupId), parseInt(projectId), parseInt(versionId));
  };

  const handleCreateGroup = async (e) => {
    e.preventDefault();
    if (!newGroupName.trim()) return;
    try {
      await createNewGroup(newGroupName);
      setNewGroupName('');
      setShowGroupForm(false);
    } catch (err) {
      console.error('Failed to create group:', err);
    }
  };

  const handleCreateProject = async (e) => {
    e.preventDefault();
    if (!newProjectName.trim() || currentGroup == null) return;
    try {
      await createNewProject(currentGroup, newProjectName);
      setNewProjectName('');
      setShowProjectForm(false);
    } catch (err) {
      console.error('Failed to create project:', err);
    }
  };

  const handleCreateVersion = async (e) => {
    e.preventDefault();
    if (!newVersionName.trim() || currentGroup == null || currentProject == null) return;
    try {
      await createNewVersion(currentGroup, currentProject, newVersionName);
      setNewVersionName('');
      setShowVersionForm(false);
    } catch (err) {
      console.error('Failed to create version:', err);
    }
  };

  const handleCreateScan = async (e) => {
    e.preventDefault();
    if (!newScanName.trim() || currentGroup == null || currentProject == null || currentVersion == null) return;
    try {
      await createNewScan(currentGroup, currentProject, currentVersion, newScanName);
      setNewScanName('');
      setShowScanForm(false);
    } catch (err) {
      console.error('Failed to create scan:', err);
    }
  };

  const handleDeleteGroup = async (groupId) => {
    if (window.confirm('Are you sure you want to delete this group and all its projects?')) {
      try {
        await deleteExistingGroup(groupId);
        if (currentGroup === groupId) setCurrentGroup(null);
      } catch (err) {
        console.error('Failed to delete group:', err);
      }
    }
  };

  const handleDeleteProject = async (projectId) => {
    if (window.confirm('Are you sure you want to delete this project and all its versions?')) {
      try {
        await deleteExistingProject(currentGroup, projectId);
        if (currentProject === projectId) setCurrentProject(null);
      } catch (err) {
        console.error('Failed to delete project:', err);
      }
    }
  };

  const handleDeleteVersion = async (versionId) => {
    if (window.confirm('Are you sure you want to delete this version and all its scans?')) {
      try {
        await deleteExistingVersion(currentGroup, currentProject, versionId);
        if (currentVersion === versionId) setCurrentVersion(null);
      } catch (err) {
        console.error('Failed to delete version:', err);
      }
    }
  };

  // Filter projects for current group
  const currentGroupProjects = Object.entries(projects).filter(([id, name]) => id);

  // Filter versions for current project
  const currentProjectVersions = Object.entries(versions).filter(([id, name]) => id);

  // Filter scans for current version
  const currentVersionScans = Object.entries(scans).filter(([id, name]) => id);

  return (
    <div className="treeview-container">
      <div className="treeview-header">
        <h3>Project Structure</h3>
      </div>

      {/* Groups Section */}
      <div className="treeview-section">
        <div className="section-header">
          <h4>Groups</h4>
          <button
            className="btn-add"
            onClick={() => setShowGroupForm(!showGroupForm)}
            title="Add group"
            disabled={loading}
          >
            +
          </button>
        </div>

        {showGroupForm && (
          <form className="quick-form" onSubmit={handleCreateGroup}>
            <input
              type="text"
              value={newGroupName}
              onChange={(e) => setNewGroupName(e.target.value)}
              placeholder="Group name"
              disabled={loading}
              autoFocus
            />
            <div className="form-buttons">
              <button type="submit" className="btn-mini" disabled={loading || !newGroupName.trim()}>
                Create
              </button>
              <button
                type="button"
                className="btn-mini-cancel"
                onClick={() => {
                  setShowGroupForm(false);
                  setNewGroupName('');
                }}
              >
                Cancel
              </button>
            </div>
          </form>
        )}

        {Object.keys(groups).length === 0 ? (
          <div className="empty-tree-message">No groups yet</div>
        ) : (
          <div className="tree-items">
            {Object.entries(groups).map(([groupId, groupName]) => (
              <div key={groupId} className="tree-node">
                <div
                  className={`tree-node-content ${currentGroup === parseInt(groupId) ? 'active' : ''}`}
                  onClick={() => handleSelectGroup(groupId)}
                >
                  <span className="tree-expander">{expandedGroups[groupId] ? '▼' : '▶'}</span>
                  <span className="tree-label">{groupName}</span>
                  <button
                    className="btn-delete-node"
                    onClick={(e) => {
                      e.stopPropagation();
                      handleDeleteGroup(groupId);
                    }}
                    title="Delete"
                  >
                    ✕
                  </button>
                </div>

                {expandedGroups[groupId] && currentGroup === parseInt(groupId) && (
                  <div className="tree-children">
                    {/* Projects for this group */}
                    {currentGroupProjects.length === 0 ? (
                      <div className="tree-empty">No projects</div>
                    ) : (
                      currentGroupProjects.map(([projectId, projectName]) => (
                        <div key={projectId} className="tree-node">
                          <div
                            className={`tree-node-content ${currentProject === parseInt(projectId) ? 'active' : ''}`}
                            onClick={() => handleSelectProject(projectId, groupId)}
                          >
                            <span className="tree-expander">{expandedProjects[projectId] ? '▼' : '▶'}</span>
                            <span className="tree-label">{projectName}</span>
                            <button
                              className="btn-delete-node"
                              onClick={(e) => {
                                e.stopPropagation();
                                handleDeleteProject(projectId);
                              }}
                              title="Delete"
                            >
                              ✕
                            </button>
                          </div>

                          {expandedProjects[projectId] && currentProject === parseInt(projectId) && (
                            <div className="tree-children">
                              {/* Versions for this project */}
                              {currentProjectVersions.length === 0 ? (
                                <div className="tree-empty">No versions</div>
                              ) : (
                                currentProjectVersions.map(([versionId, versionName]) => (
                                  <div key={versionId} className="tree-node">
                                    <div
                                      className={`tree-node-content ${currentVersion === parseInt(versionId) ? 'active' : ''}`}
                                      onClick={() => handleSelectVersion(versionId, groupId, projectId)}
                                    >
                                      <span className="tree-expander">{expandedVersions[versionId] ? '▼' : '▶'}</span>
                                      <span className="tree-label">{versionName}</span>
                                      <button
                                        className="btn-delete-node"
                                        onClick={(e) => {
                                          e.stopPropagation();
                                          handleDeleteVersion(versionId);
                                        }}
                                        title="Delete"
                                      >
                                        ✕
                                      </button>
                                    </div>

                                    {expandedVersions[versionId] && currentVersion === parseInt(versionId) && (
                                      <div className="tree-children">
                                        {/* Scans for this version */}
                                        {currentVersionScans.length === 0 ? (
                                          <div className="tree-empty">No scans</div>
                                        ) : (
                                          currentVersionScans.map(([scanId, scanName]) => (
                                            <div key={scanId} className="tree-node">
                                              <div
                                                className={`tree-node-content ${currentScan === parseInt(scanId) ? 'active' : ''}`}
                                                onClick={() => setCurrentScan(parseInt(scanId))}
                                              >
                                                <span className="tree-expander">•</span>
                                                <span className="tree-label">{scanName}</span>
                                              </div>
                                            </div>
                                          ))
                                        )}

                                        {/* Quick add scan form */}
                                        {showScanForm && (
                                          <form className="quick-form nested" onSubmit={handleCreateScan}>
                                            <input
                                              type="text"
                                              value={newScanName}
                                              onChange={(e) => setNewScanName(e.target.value)}
                                              placeholder="Scan name"
                                              disabled={loading}
                                              autoFocus
                                            />
                                            <div className="form-buttons">
                                              <button
                                                type="submit"
                                                className="btn-mini"
                                                disabled={loading || !newScanName.trim()}
                                              >
                                                Create
                                              </button>
                                              <button
                                                type="button"
                                                className="btn-mini-cancel"
                                                onClick={() => {
                                                  setShowScanForm(false);
                                                  setNewScanName('');
                                                }}
                                              >
                                                Cancel
                                              </button>
                                            </div>
                                          </form>
                                        )}

                                        {!showScanForm && (
                                          <button
                                            className="btn-add-nested"
                                            onClick={() => setShowScanForm(true)}
                                            disabled={loading}
                                          >
                                            + Add Scan
                                          </button>
                                        )}
                                      </div>
                                    )}
                                  </div>
                                ))
                              )}

                              {/* Quick add version form */}
                              {showVersionForm && (
                                <form className="quick-form nested" onSubmit={handleCreateVersion}>
                                  <input
                                    type="text"
                                    value={newVersionName}
                                    onChange={(e) => setNewVersionName(e.target.value)}
                                    placeholder="Version name"
                                    disabled={loading}
                                    autoFocus
                                  />
                                  <div className="form-buttons">
                                    <button
                                      type="submit"
                                      className="btn-mini"
                                      disabled={loading || !newVersionName.trim()}
                                    >
                                      Create
                                    </button>
                                    <button
                                      type="button"
                                      className="btn-mini-cancel"
                                      onClick={() => {
                                        setShowVersionForm(false);
                                        setNewVersionName('');
                                      }}
                                    >
                                      Cancel
                                    </button>
                                  </div>
                                </form>
                              )}

                              {!showVersionForm && (
                                <button
                                  className="btn-add-nested"
                                  onClick={() => setShowVersionForm(true)}
                                  disabled={loading}
                                >
                                  + Add Version
                                </button>
                              )}
                            </div>
                          )}
                        </div>
                      ))
                    )}

                    {/* Quick add project form */}
                    {showProjectForm && (
                      <form className="quick-form nested" onSubmit={handleCreateProject}>
                        <input
                          type="text"
                          value={newProjectName}
                          onChange={(e) => setNewProjectName(e.target.value)}
                          placeholder="Project name"
                          disabled={loading}
                          autoFocus
                        />
                        <div className="form-buttons">
                          <button
                            type="submit"
                            className="btn-mini"
                            disabled={loading || !newProjectName.trim()}
                          >
                            Create
                          </button>
                          <button
                            type="button"
                            className="btn-mini-cancel"
                            onClick={() => {
                              setShowProjectForm(false);
                              setNewProjectName('');
                            }}
                          >
                            Cancel
                          </button>
                        </div>
                      </form>
                    )}

                    {!showProjectForm && (
                      <button
                        className="btn-add-nested"
                        onClick={() => setShowProjectForm(true)}
                        disabled={loading}
                      >
                        + Add Project
                      </button>
                    )}
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
};
