import React, { createContext, useContext, useState, useCallback } from 'react';
import api from '../services/api';

const ScannerContext = createContext();

export const ScannerProvider = ({ children }) => {
  // Navigation/Selection State
  const [currentGroup, setCurrentGroup] = useState(null);
  const [currentProject, setCurrentProject] = useState(null);
  const [currentVersion, setCurrentVersion] = useState(null);
  const [currentScan, setCurrentScan] = useState(null);

  // Data State
  const [groups, setGroups] = useState({});
  const [projects, setProjects] = useState({});
  const [versions, setVersions] = useState({});
  const [scans, setScans] = useState({});

  // UI State
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [scanInfo, setScanInfo] = useState(null);

  // Helper to clear error after timeout
  const setErrorWithTimeout = (err, timeout = 5000) => {
    setError(err);
    if (timeout > 0) {
      setTimeout(() => setError(null), timeout);
    }
  };

  // ===== GROUPS =====
  const loadGroups = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.listGroups();
      setGroups(data || {});
      return data || {};
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const createNewGroup = useCallback(async (name) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.createGroup(name);
      await loadGroups();
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadGroups]);

  const updateExistingGroup = useCallback(async (groupId, name) => {
    try {
      setLoading(true);
      setError(null);
      await api.updateGroup(groupId, name);
      await loadGroups();
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadGroups]);

  const deleteExistingGroup = useCallback(async (groupId) => {
    try {
      setLoading(true);
      setError(null);
      await api.deleteGroup(groupId);
      if (currentGroup === groupId) setCurrentGroup(null);
      await loadGroups();
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [currentGroup, loadGroups]);

  // ===== PROJECTS =====
  const loadProjects = useCallback(async (groupId) => {
    if (groupId == null) return;
    try {
      setLoading(true);
      setError(null);
      const data = await api.listProjects(groupId);
      setProjects(data || {});
      return data || {};
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const createNewProject = useCallback(async (groupId, name) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.createProject(groupId, name);
      await loadProjects(groupId);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadProjects]);

  const updateExistingProject = useCallback(async (groupId, projectId, name) => {
    try {
      setLoading(true);
      setError(null);
      await api.updateProject(groupId, projectId, name);
      await loadProjects(groupId);
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadProjects]);

  const deleteExistingProject = useCallback(async (groupId, projectId) => {
    try {
      setLoading(true);
      setError(null);
      await api.deleteProject(groupId, projectId);
      if (currentProject === projectId) setCurrentProject(null);
      await loadProjects(groupId);
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [currentProject, loadProjects]);

  // ===== VERSIONS =====
  const loadVersions = useCallback(async (groupId, projectId) => {
    if (groupId == null || projectId == null) return;
    try {
      setLoading(true);
      setError(null);
      const data = await api.listVersions(groupId, projectId);
      setVersions(data || {});
      return data || {};
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const createNewVersion = useCallback(async (groupId, projectId, name) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.createVersion(groupId, projectId, name);
      await loadVersions(groupId, projectId);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadVersions]);

  const updateExistingVersion = useCallback(async (groupId, projectId, versionId, name) => {
    try {
      setLoading(true);
      setError(null);
      await api.updateVersion(groupId, projectId, versionId, name);
      await loadVersions(groupId, projectId);
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadVersions]);

  const deleteExistingVersion = useCallback(async (groupId, projectId, versionId) => {
    try {
      setLoading(true);
      setError(null);
      await api.deleteVersion(groupId, projectId, versionId);
      if (currentVersion === versionId) setCurrentVersion(null);
      await loadVersions(groupId, projectId);
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [currentVersion, loadVersions]);

  // ===== SCANS =====
  const loadScans = useCallback(async (groupId, projectId, versionId) => {
    if (groupId == null || projectId == null || versionId == null) return;
    try {
      setLoading(true);
      setError(null);
      const data = await api.listScans(groupId, projectId, versionId);
      setScans(data || {});
      return data || {};
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const createNewScan = useCallback(async (groupId, projectId, versionId, name) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.createScan(groupId, projectId, versionId, name);
      await loadScans(groupId, projectId, versionId);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadScans]);

  const deleteExistingScan = useCallback(async (groupId, projectId, versionId, scanId) => {
    try {
      setLoading(true);
      setError(null);
      await api.deleteScan(groupId, projectId, versionId, scanId);
      if (currentScan === scanId) setCurrentScan(null);
      await loadScans(groupId, projectId, versionId);
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [currentScan, loadScans]);

  const loadScanInfo = useCallback(async (groupId, projectId, versionId, scanId) => {
    if (groupId == null || projectId == null || versionId == null || scanId == null) return;
    try {
      setLoading(true);
      setError(null);
      const data = await api.getScanInfo(groupId, projectId, versionId, scanId);
      setScanInfo(data);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const startExistingScan = useCallback(async (groupId, projectId, versionId, scanId) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.startScan(groupId, projectId, versionId, scanId);
      await loadScanInfo(groupId, projectId, versionId, scanId);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadScanInfo]);

  const abortExistingScan = useCallback(async (groupId, projectId, versionId, scanId) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.abortScan(groupId, projectId, versionId, scanId);
      await loadScanInfo(groupId, projectId, versionId, scanId);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, [loadScanInfo]);

  const uploadFile = useCallback(async (groupId, projectId, versionId, scanId, file) => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.uploadScanFile(groupId, projectId, versionId, scanId, file);
      return data;
    } catch (err) {
      setErrorWithTimeout(err.message);
      throw err;
    } finally {
      setLoading(false);
    }
  }, []);

  const value = {
    // Navigation
    currentGroup,
    setCurrentGroup,
    currentProject,
    setCurrentProject,
    currentVersion,
    setCurrentVersion,
    currentScan,
    setCurrentScan,

    // Data
    groups,
    projects,
    versions,
    scans,
    scanInfo,

    // UI State
    loading,
    error,

    // Group Actions
    loadGroups,
    createNewGroup,
    updateExistingGroup,
    deleteExistingGroup,

    // Project Actions
    loadProjects,
    createNewProject,
    updateExistingProject,
    deleteExistingProject,

    // Version Actions
    loadVersions,
    createNewVersion,
    updateExistingVersion,
    deleteExistingVersion,

    // Scan Actions
    loadScans,
    createNewScan,
    deleteExistingScan,
    startExistingScan,
    abortExistingScan,
    loadScanInfo,
    uploadFile,
  };

  return <ScannerContext.Provider value={value}>{children}</ScannerContext.Provider>;
};

export const useScanner = () => {
  const context = useContext(ScannerContext);
  if (!context) {
    throw new Error('useScanner must be used within ScannerProvider');
  }
  return context;
};
