// API Service for AhnalyticScanner
// Handles all backend communication with the ScanServer

const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://127.0.0.1:8080';

class ApiService {
  // Helper method for API calls
  async request(method, endpoint, body = null) {
    const options = {
      method,
      headers: {
        'Content-Type': 'application/json',
      },
    };

    if (body) {
      options.body = JSON.stringify(body);
    }

    try {
      const response = await fetch(`${API_BASE_URL}${endpoint}`, options);
      
      if (!response.ok) {
        const error = await response.json().catch(() => ({ error: 'Unknown error' }));
        throw new Error(error.error || `HTTP ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error(`API Error [${method} ${endpoint}]:`, error);
      throw error;
    }
  }

  // ===== GROUPS =====
  createGroup(name) {
    return this.request('POST', '/groups', { name });
  }

  listGroups() {
    return this.request('GET', '/groups');
  }

  updateGroup(groupId, name) {
    return this.request('PUT', `/groups/${groupId}`, { name });
  }

  deleteGroup(groupId) {
    return this.request('DELETE', `/groups/${groupId}`);
  }

  // ===== PROJECTS =====
  createProject(groupId, name) {
    return this.request('POST', `/groups/${groupId}/projects`, { name });
  }

  listProjects(groupId) {
    return this.request('GET', `/groups/${groupId}/projects`);
  }

  updateProject(groupId, projectId, name) {
    return this.request('PUT', `/groups/${groupId}/projects/${projectId}`, { name });
  }

  deleteProject(groupId, projectId) {
    return this.request('DELETE', `/groups/${groupId}/projects/${projectId}`);
  }

  // ===== VERSIONS =====
  createVersion(groupId, projectId, name) {
    return this.request('POST', `/groups/${groupId}/projects/${projectId}/versions`, { name });
  }

  listVersions(groupId, projectId) {
    return this.request('GET', `/groups/${groupId}/projects/${projectId}/versions`);
  }

  updateVersion(groupId, projectId, versionId, name) {
    return this.request('PUT', `/groups/${groupId}/projects/${projectId}/versions/${versionId}`, { name });
  }

  deleteVersion(groupId, projectId, versionId) {
    return this.request('DELETE', `/groups/${groupId}/projects/${projectId}/versions/${versionId}`);
  }

  // ===== SCANS =====
  createScan(groupId, projectId, versionId, name) {
    return this.request('POST', `/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans`, { name });
  }

  listScans(groupId, projectId, versionId) {
    return this.request('GET', `/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans`);
  }

  startScan(groupId, projectId, versionId, scanId) {
    return this.request('POST', `/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans/${scanId}/start`);
  }

  abortScan(groupId, projectId, versionId, scanId) {
    return this.request('POST', `/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans/${scanId}/abort`);
  }

  getScanInfo(groupId, projectId, versionId, scanId) {
    return this.request('GET', `/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans/${scanId}/info`);
  }

  // File upload for scan
  // Note: This assumes the backend has a file upload endpoint
  // If not, you may need to adjust this based on your backend implementation
  async uploadScanFile(groupId, projectId, versionId, scanId, file) {
    const formData = new FormData();
    formData.append('file', file);

    try {
      const response = await fetch(
        `${API_BASE_URL}/groups/${groupId}/projects/${projectId}/versions/${versionId}/scans/${scanId}/upload`,
        {
          method: 'POST',
          body: formData,
        }
      );

      if (!response.ok) {
        throw new Error(`Upload failed with status ${response.status}`);
      }

      return await response.json();
    } catch (error) {
      console.error('File upload error:', error);
      throw error;
    }
  }
}

export default new ApiService();
